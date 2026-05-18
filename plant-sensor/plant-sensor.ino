// --- HARDWARE CONFIGURATION ---
const int MOISTURE_PIN = 13; 
const int LED_PIN = LED_BUILTIN; // Uses the on-board LED. Change to an external pin if needed!

// --- CALIBRATION THRESHOLDS ---
// TODO: YOU WILL have to fine tune these levels, this worked for me
const int DRY_VALUE = 2920;  // Open-air/Dry soil baseline
const int WET_VALUE = 1800;  // Submerged/Wet soil baseline
const int MOISTURE_THRESHOLD_PCT = 25; // Alert if moisture drops below this %

// --- TIMING CONFIGURATION (In Seconds) ---
const unsigned long POLLING_INTERVAL_S = 1800; // How often to wake up and check (e.g., 60 for 1 minute, 1800 for 30 mins, 3600 for 1 hour)
const unsigned long ALERT_BLINK_DURATION_S = 5; // How long to blink the LED if dry

void setup() {
  Serial.begin(115200);
  
  // Brief delay to allow Serial Monitor to attach if plugged into USB
  delay(1000); 
  Serial.println("\n--- Waking up from sleep... ---");

  // Configure the LED pin as an output
  pinMode(LED_PIN, OUTPUT);

  // 1. Take a smoothed sensor reading
  int total = 0;
  for (int i = 0; i < 10; i++) {
    total += analogRead(MOISTURE_PIN);
    delay(10); 
  }
  int smoothedValue = total / 10;

  // 2. Calculate moisture percentage
  int moisturePercent = map(smoothedValue, DRY_VALUE, WET_VALUE, 0, 100);
  moisturePercent = constrain(moisturePercent, 0, 100);

  Serial.print("Current Soil Moisture: ");
  Serial.print(moisturePercent);
  Serial.println("%");

  // 3. Action Phase: If it's dry, blink the light
  if (moisturePercent <= MOISTURE_THRESHOLD_PCT) {
    Serial.println("Soil is DRY! Initiating alert blink...");
    
    // Calculate how many times to loop the blink to fill the target seconds
    // Each loop iteration takes 500ms total (250ms ON + 250ms OFF), so 2 blinks per second
    int totalBlinks = ALERT_BLINK_DURATION_S * 2;
    
    for (int i = 0; i < totalBlinks; i++) {
      digitalWrite(LED_PIN, HIGH); // Turn LED on
      delay(250);
      digitalWrite(LED_PIN, LOW);  // Turn LED off
      delay(250);
    }
  } else {
    Serial.println("Soil is moist enough. No alert needed.");
  }

  // 4. Sleep Phase: Calculate sleep time and go under
  Serial.print("Going to deep sleep for ");
  Serial.print(POLLING_INTERVAL_S);
  Serial.println(" seconds...");
  Serial.flush(); // Ensure all serial text is transmitted before shutting down the chip

  // Convert seconds to microseconds for the ESP32 internal timer API
  esp_sleep_enable_timer_wakeup(POLLING_INTERVAL_S * 1000000ULL);
  
  // Put the chip into deep sleep mode
  esp_deep_sleep_start();
}

void loop() {
  // This remains totally empty! Execution will never reach here because 
  // the chip deep-sleeps at the end of setup() and restarts from setup() when it wakes up.
}