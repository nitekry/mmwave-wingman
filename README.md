# mmwave-wingman
Using a mmwave RD-03D 24GHz radar module for stand alone, real-time human detection. 
Using M5 Atom S3 Lite

https://www.digikey.com/en/products/detail/ai-thinker/RD-03D/24614840


1. Baseline Behavior (Raw Output Only)
- Printed full RX_BUF frames upon receiving complete radar data.
- Extracted distance, X, Y, speed, and distance resolution.
- No validation or filtering—every frame was processed.

2. Environmental & Signal Filtering
- Ignored stale data: Sticky frames after human presence. 

  Frames where all values matched the previous frame (X, Y, speed, distance).

  Prevented redundant logging and false positives from static environments.

- Discarded invalid readings: In the end ignoring Y completly so this needs some work. 

 Y == -32768 (0x8000): indicates default or garbage value.

 Speed == -16 (0x0010 raw): also used as a default placeholder.

- Distance cutoff: limiting the alert range to maximum realistic distance. 

  Readings over 12 meters are considered noise and ignored. 

3. Adaptive Repeat Filtering
- Dynamic filtering threshold (repeatCount, maxRepeatThreshold) adjusts based on repeated identical frames.
- Prevents persistent junk from being processed as real detections.

4. Buzzer Alert System
- Added support for active-low buzzer on pin 7.
- Beeps once every 3 seconds only if movement is detected.
- Automatically silenced when no valid movement occurs.

5. Unit Conversion and Display Logic
- Converted raw distance to meters using:
  float distanceMeters = sqrt(x*x + y*y) / 1000.0; //Just easier to read scrolling data

    Serial Print

  Raw frame (hex).
  Distance in meters.
  Angle from X-axis (derived from atan2(x, y)).
  Excluded Y from display for clarity.

6. Frame Management
- Frame validation using header and tail (0xAA 0xFF ... 0x55 0xCC).
- Automatically resets buffer index on overflow or completion.  //a lot of sticky data was causing alerts.
- Sends single-target detection command every 5 seconds to maintain active polling. //still testing the frequency that works best.


![mmwave-device](https://github.com/user-attachments/assets/ae3cfe68-6442-4895-b82e-0178efc00ab0)

