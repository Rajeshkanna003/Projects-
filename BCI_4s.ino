#include "arm_math.h"
#include <math.h>

#define SAMPLE_RATE 512
#define FFT_SIZE 256
#define BAUD_RATE 115200
#define INPUT_PIN A0

// EEG Frequency Bands
#define DELTA_LOW 0.5
#define DELTA_HIGH 4.0 
#define THETA_LOW 4.0
#define THETA_HIGH 8.0
#define ALPHA_LOW 8.0
#define ALPHA_HIGH 13.0
#define BETA_LOW 13.0
#define BETA_HIGH 30.0
#define GAMMA_LOW 30.0
#define GAMMA_HIGH 45.0

#define SMOOTHING_FACTOR 0.63
const float EPS = 1e-6f;

// Duration to confirm focus or distraction (ms)
#define FOCUS_HOLD_DURATION 4000UL

typedef struct {
  float delta, theta, alpha, beta, gamma, total;
} BandpowerResults, SmoothedBandpower;

SmoothedBandpower smoothedPowers = {0};

unsigned long timerStart = 0;
bool ledState = false;
bool previousBetaAbove = false;

float Notch(float input) {
  float output = input;
  {
    static float z1 = 0, z2 = 0;
    float x = output - (-1.56858163f * z1) - (0.96424138f * z2);
    output = 0.96508099f * x + (-1.56202714f * z1) + (0.96508099f * z2);
    z2 = z1;
    z1 = x;
  }
  {
    static float z1 = 0, z2 = 0;
    float x = output - (-1.61100358f * z1) - (0.96592171f * z2);
    output = 1.00000000f * x + (-1.61854514f * z1) + (1.00000000f * z2);
    z2 = z1;
    z1 = x;
  }
  return output;
}

float EEGFilter(float input) {
  float output = input;
  {
    static float z1, z2;
    float x = output - -1.22465158 * z1 - 0.45044543 * z2;
    output = 0.05644846 * x + 0.11289692 * z1 + 0.05644846 * z2;
    z2 = z1;
    z1 = x;
  }
  return output;
}

float inputBuffer[FFT_SIZE];
float fftOutputBuffer[FFT_SIZE];
float powerSpectrum[FFT_SIZE / 2];

arm_rfft_fast_instance_f32 S;
volatile uint16_t sampleIndex = 0;
volatile bool bufferReady = false;

void smoothBandpower(BandpowerResults *raw, SmoothedBandpower *smoothed) {
  smoothed->delta = SMOOTHING_FACTOR * raw->delta + (1 - SMOOTHING_FACTOR) * smoothed->delta;
  smoothed->theta = SMOOTHING_FACTOR * raw->theta + (1 - SMOOTHING_FACTOR) * smoothed->theta;
  smoothed->alpha = SMOOTHING_FACTOR * raw->alpha + (1 - SMOOTHING_FACTOR) * smoothed->alpha;
  smoothed->beta = SMOOTHING_FACTOR * raw->beta + (1 - SMOOTHING_FACTOR) * smoothed->beta;
  smoothed->gamma = SMOOTHING_FACTOR * raw->gamma + (1 - SMOOTHING_FACTOR) * smoothed->gamma;
  smoothed->total = SMOOTHING_FACTOR * raw->total + (1 - SMOOTHING_FACTOR) * smoothed->total;
}

BandpowerResults calculateBandpower(float *powerSpectrum, float binResolution, uint16_t halfSize) {
  BandpowerResults results = {0};

  for (uint16_t i = 1; i < halfSize; i++) {
    float freq = i * binResolution;
    float power = powerSpectrum[i];
    results.total += power;

    if (freq >= DELTA_LOW && freq < DELTA_HIGH) results.delta += power;
    else if (freq >= THETA_LOW && freq < THETA_HIGH) results.theta += power;
    else if (freq >= ALPHA_LOW && freq < ALPHA_HIGH) results.alpha += power;
    else if (freq >= BETA_LOW && freq < BETA_HIGH) results.beta += power;
    else if (freq >= GAMMA_LOW && freq < GAMMA_HIGH) results.gamma += power;
  }

  return results;
}

void processFFT() {
  arm_rfft_fast_f32(&S, inputBuffer, fftOutputBuffer, 0);

  uint16_t halfSize = FFT_SIZE / 2;
  for (uint16_t i = 0; i < halfSize; i++) {
    float real = fftOutputBuffer[2 * i];
    float imag = fftOutputBuffer[2 * i + 1];
    powerSpectrum[i] = real * real + imag * imag;
  }

  float binResolution = (float)SAMPLE_RATE / FFT_SIZE;
  BandpowerResults rawBandpower = calculateBandpower(powerSpectrum, binResolution, halfSize);
  smoothBandpower(&rawBandpower, &smoothedPowers);

  float betaRatio = (smoothedPowers.beta / (smoothedPowers.total + EPS)) * 100;
  bool betaAbove = betaRatio > 25.0;

  unsigned long currentTime = millis();

  if (betaAbove) {
    if (!previousBetaAbove) {
      timerStart = currentTime;  // Start counting now
    } else if (currentTime - timerStart >= FOCUS_HOLD_DURATION) {
      digitalWrite(LED_BUILTIN, HIGH);
      ledState = true;
    }
  } else {
    if (previousBetaAbove) {
      timerStart = currentTime;  // Start counting now
    } else if (currentTime - timerStart >= FOCUS_HOLD_DURATION) {
      digitalWrite(LED_BUILTIN, LOW);
      ledState = false;
    }
  }

  previousBetaAbove = betaAbove;

  Serial.print("Delta: "); Serial.print((smoothedPowers.delta / (smoothedPowers.total + EPS)) * 100); Serial.print(", ");
  Serial.print("Theta: "); Serial.print((smoothedPowers.theta / (smoothedPowers.total + EPS)) * 100); Serial.print(", ");
  Serial.print("Alpha: "); Serial.print((smoothedPowers.alpha / (smoothedPowers.total + EPS)) * 100); Serial.print(", ");
  Serial.print("Beta: ");  Serial.print(betaRatio); Serial.print(", ");
  Serial.print("Gamma: "); Serial.print((smoothedPowers.gamma / (smoothedPowers.total + EPS)) * 100); Serial.print(" | ");
  Serial.print("LED: "); Serial.println(ledState ? "ON" : "OFF");
}

void setup() {
  Serial.begin(BAUD_RATE);
  while (!Serial);
  pinMode(INPUT_PIN, INPUT);
  pinMode(LED_BUILTIN, OUTPUT); // or pinMode(13, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  arm_rfft_fast_init_f32(&S, FFT_SIZE);
}

void loop() {
  static unsigned long lastMicros = 0;
  unsigned long currentMicros = micros();
  unsigned long interval = currentMicros - lastMicros;
  lastMicros = currentMicros;

  static long timer = 0;
  timer -= interval;
  if (timer < 0) {
    timer += 1000000 / SAMPLE_RATE;

    int rawSample = analogRead(INPUT_PIN);
    float filteredSample = EEGFilter(Notch(rawSample));

    if (sampleIndex < FFT_SIZE) {
      inputBuffer[sampleIndex++] = filteredSample;
    }
    if (sampleIndex >= FFT_SIZE) {
      bufferReady = true;
    }
  }

  if (bufferReady) {
    processFFT();
    sampleIndex = 0;
    bufferReady = false;
  }
}
