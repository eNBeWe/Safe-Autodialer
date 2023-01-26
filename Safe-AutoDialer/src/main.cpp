#include <Arduino.h>
#include <AccelStepper.h>

int willPickUpDisk(uint8 disk, uint8 targetPosition, uint8 direction, uint8 fullRotation);
void testOpen();
void rotateDial(uint8 position, uint8 direction);
void rotateFull(uint8 direction);
void dialCombination(uint8 disk0, uint8 disk1, uint8 disk2);
void setDisk2(uint8 position);
void setDisk1(uint8 position);
void setDisk0(uint8 position);
void printAllDiskState();
void printDiskState(uint8 disk);

// Define stepper motor connections and motor interface type. Motor interface type must be set to 1 when using a driver
#define motorInterfaceType 1
#define dirPin D2
#define stepPin D3

// Stepper motor speed configuration
#define microStepsFactor 8
#define maxStepperSpeed 4000
#define inversionPause 50

// Create a new instance of the AccelStepper class:
AccelStepper stepper = AccelStepper(motorInterfaceType, stepPin, dirPin);

// Runtime data
uint8 currentDialPosition = 255;
uint8 currentDiskPosition[] = {255, 255, 255};
uint8 rotationMode[] = {0, 0, 0};
uint8 startOffset = 0;

void setup() {
  Serial.begin(115200);
  // Set the maximum speed in steps per second:
  stepper.setMaxSpeed(maxStepperSpeed);
  stepper.setAcceleration(40000);  

  Serial.println("Usage:");
  Serial.println("==================");
  Serial.println("1 - One full rotation counter-clockwise");
  Serial.println("2 - One full rotation clockwise");
  Serial.println("3 - Quarter rotation counter-clockwise");
  Serial.println("4 - Quarter rotation clockwise");
  Serial.println("5 - Rotate one number counter-clockwise");
  Serial.println("6 - Rotate one number clockwise");
  Serial.println("p - Rotate couter-clockwise three times to pick up all disks");
  Serial.println("0 - Set current dial position as 0");
  Serial.println("+ - Increment start number for first disk");
  Serial.println("o - Try opening by rotating counterclockwise almost one turn");
  Serial.println("s - Start autodialing numbers");
}

void loop() { 
  if (Serial.available()) {
    int input = Serial.read();

    if (input == '1') {
      Serial.println("Doing a full rotation counter-clockwise");
      rotateFull(2);
    } else if (input == '2') {
      Serial.println("Doing a full rotation clockwise");
      rotateFull(1);
    } else if (input == '3') {
      Serial.println("Doing a quarter rotation counter-clockwise");
      rotateDial((currentDialPosition + 25) % 100, 2);
    } else if (input == '4') {
      Serial.println("Doing a quarter rotation clockwise");
      rotateDial((currentDialPosition + 75) % 100, 1);
    } else if (input == '5') {
      Serial.println("Moving one number counter-clockwise");
      rotateDial((currentDialPosition + 1) % 100, 2);
    } else if (input == '6') {
      Serial.println("Moving one number clockwise");
      rotateDial((currentDialPosition + 99) % 100, 1);
    } else if (input == 'p') {
      Serial.println("Rotating three times couter-clockwise to pick up all disks");
      rotateFull(2);
      rotateFull(2);
      rotateFull(2);
    } else if (input == '0') {
      Serial.println("Current dial position stored as 0.");
      stepper.setCurrentPosition(0);
      currentDialPosition = 0;
    } else if (input == '+') {
      startOffset += 1;
      Serial.print("Start offset incremented to ");
      Serial.println(startOffset);
    } else if (input == 'o') {
      Serial.println("Try opening by rotating counterclockwise almost one turn");
      testOpen();
    } else if (input == 's') {
      Serial.println("Starting to test combinations");
      for (uint8 k = startOffset; k < 100; k+=2) {
        for (uint8 j = 98; j < 100; j-=2) {
          for (uint8 i = 0; i < 100; i+=2) {
            Serial.print("Testing combination ");
            Serial.print(k);
            Serial.print(" - ");
            Serial.print(j);
            Serial.print(" - ");
            Serial.println(i);
            dialCombination(k, j, i);
            testOpen();
          }
        }
      }
    }
  }
  delay(200);
}

/**
 * Predicate to determine whether moving the dial to <targetPosition> 
 * in direction <direction> will change the position of disk number <disk>.
 * If <fullRotation> is true, a movement to the current dial position is interpreted
 * as a full rotation instead of no rotation.
 */
int willPickUpDisk(uint8 disk, uint8 targetPosition, uint8 direction, uint8 fullRotation) {
  uint8 startPosition = disk == 0 ? currentDialPosition : currentDiskPosition[disk - 1];
  uint8 drivingDiskRotationMode = disk == 0 ? direction : rotationMode[disk - 1];

  if (startPosition < targetPosition) {
    return (direction == 2 && (currentDiskPosition[disk] > startPosition && currentDiskPosition[disk] < targetPosition))
        || (direction == 1 && (currentDiskPosition[disk] < startPosition || currentDiskPosition[disk] > targetPosition))
        || (currentDiskPosition[disk] == targetPosition)
        || (currentDiskPosition[disk] == startPosition && rotationMode[disk] == direction);
  } else if (startPosition > targetPosition) {
    return (direction == 2 && (currentDiskPosition[disk] > startPosition || currentDiskPosition[disk] < targetPosition))
        || (direction == 1 && (currentDiskPosition[disk] < startPosition && currentDiskPosition[disk] > targetPosition))
        || (currentDiskPosition[disk] == targetPosition)
        || (currentDiskPosition[disk] == startPosition && rotationMode[disk] == direction);
  } else if (fullRotation == 1) {
    // Start is the same as end, with full rotation
    return drivingDiskRotationMode == direction;
  } else {
    // Start is the same as end without movement, just check if already picked up.
    return rotationMode[disk] == direction;
  }
}

/**
 * Try opening the lock by moving close to a full rotation clockwise,
 * wait a moment and rotate back to the previous position.
 */
void testOpen() {
  rotateDial((currentDialPosition + 5) % 100, 1);
  delay(100);
  rotateDial((currentDialPosition + 95) % 100, 2);
}

/**
 * Rotate the dial to <position>, moving in the given <direction>.
 * If <direction> is 1 the rotation is done clockwise,
 * if it is 2 the rotation is counter-clockwise.
 */
void rotateDial(uint8 position, uint8 direction) {
  if (direction == 1) {
    Serial.print("  - Clockwise rotation of dial from position ");
    Serial.print(currentDialPosition);
    Serial.print(" to position ");
    Serial.println(position);
  } else if (direction == 2) {
    Serial.print("  - Counter-clockwise rotation of dial from position ");
    Serial.print(currentDialPosition);
    Serial.print(" to position ");
    Serial.println(position);
  }

  // Check which disks we will move here.
  int diskMoving[3] = {0, 0, 0};
  if (willPickUpDisk(0, position, direction, 0)) {
    Serial.println("    - Disk 0 will be moved");
    diskMoving[0] = 1;
  } 
  if (diskMoving[0] && willPickUpDisk(1, position, direction, 0)) {
    Serial.println("    - Disk 1 will be moved");
    diskMoving[1] = 1;
  }
  if (diskMoving[1] && willPickUpDisk(2, position, direction, 0)) {
    Serial.println("    - Disk 2 will be moved");
    diskMoving[2] = 1;
  }

  // Calc stepper motor movement
  int steps = 0;
  if (direction == 1 && currentDialPosition > position) {
    steps = (currentDialPosition - position) * 2 * microStepsFactor;
  }
  if (direction == 1 && currentDialPosition < position) {
    steps = (currentDialPosition + (100 - position)) * 2 * microStepsFactor;
  }
  if (direction == 2 && currentDialPosition < position) {
    steps = (position - currentDialPosition) * -2 * microStepsFactor;
  }
  if (direction == 2 && currentDialPosition > position) {
    steps = ((100 - currentDialPosition) + position) * -2 * microStepsFactor;
  }

  stepper.move(steps);
  stepper.runToPosition();

  // Store new dial position
  currentDialPosition = position;
  // Save new position for moved disk
  if (diskMoving[0]) {
    rotationMode[0] = direction;
    currentDiskPosition[0] = position;
  }
  if (diskMoving[1]) {
    rotationMode[1] = direction;
    currentDiskPosition[1] = position;
  }
  if (diskMoving[2]) {
    rotationMode[2] = direction;
    currentDiskPosition[2] = position;
  }
  printAllDiskState();
}

/**
 * Do a full rotation of the dial, either clockwise or counter-clockwise.
*/
void rotateFull(uint8 direction) {
  if (direction == 1) {
    Serial.print("  - Full clockwise rotation of dial at position ");
    Serial.println(currentDialPosition);
  } else if (direction == 2) {
    Serial.print("  - Full counter-clockwise rotation of dial at position ");
    Serial.println(currentDialPosition);
  }

  // Check which disks we will move here.
  int diskMoving[3] = {0, 0, 0};
  if (willPickUpDisk(0, currentDialPosition, direction, 1)) {
    Serial.println("    - Disk 0 will be moved");
    diskMoving[0] = 1;
  } 
  if (diskMoving[0] && willPickUpDisk(1, currentDialPosition, direction, 1)) {
    Serial.println("    - Disk 1 will be moved");
    diskMoving[1] = 1;
  }
  if (diskMoving[1] && willPickUpDisk(2, currentDialPosition, direction, 1)) {
    Serial.println("    - Disk 2 will be moved");
    diskMoving[2] = 1;
  }

  // Calc stepper motor movement
  int steps = direction == 1 ? 200 * microStepsFactor : -200 * microStepsFactor;
  stepper.move(steps);
  stepper.runToPosition();

  // Save new position for moved disk
  if (diskMoving[0]) {
    rotationMode[0] = direction;
    currentDiskPosition[0] = currentDialPosition;
  }
  if (diskMoving[1]) {
    rotationMode[1] = direction;
    currentDiskPosition[1] = currentDialPosition;
  }
  if (diskMoving[2]) {
    rotationMode[2] = direction;
    currentDiskPosition[2] = currentDialPosition;
  }
  printAllDiskState();
}

/**
 * Dial the given combination.
 */
void dialCombination(uint8 disk2, uint8 disk1, uint8 disk0) {
  setDisk2(disk2);
  setDisk1(disk1);
  setDisk0(disk0);
}

/**
 * Move disk 2 to the given position counter-clockwise. 
 * If the disk is already at the given position,
 * no movement is performed.
 */
void setDisk2(uint8 position) {
  if (currentDiskPosition[2] != position || rotationMode[0] != 2) {
    Serial.print("  Repositioning disk 2 to ");
    Serial.print(position);
    Serial.println(" (counter-clockwise)");
    while (!(willPickUpDisk(0, position, 2, 0) && willPickUpDisk(1, position, 2, 0) && willPickUpDisk(2, position, 2, 0))) {
      Serial.println("    Full rotation needed");
      rotateFull(2);
    }
    rotateDial(position, 2);
  }
}

/**
 * Move disk 1 to the given position clockwise. 
 * If the disk is already at the given position,
 * no movement is performed.
 */
void setDisk1(uint8 position) {
  if (currentDiskPosition[1] != position || rotationMode[1] != 1) {
    Serial.print("  Repositioning disk 1 to ");
    Serial.print(position);
    Serial.println(" (clockwise)");
    while (!(willPickUpDisk(0, position, 1, 0) && willPickUpDisk(1, position, 1, 0))) {
      Serial.println("    Full rotation needed");
      rotateFull(1);
    }
    rotateDial(position, 1);
  }
}

/**
 * Move disk 0 to the given position counter-clockwise. 
 * If the disk is already at the given position,
 * no movement is performed.
 */
void setDisk0(uint8 position) {
  if (currentDiskPosition[0] != position || rotationMode[0] != 2) {
    Serial.print("  Repositioning disk 0 to ");
    Serial.print(position);
    Serial.println(" (counter-clockwise)");
    while (!(willPickUpDisk(0, position, 2, 0))) {
      Serial.println("    Full rotation needed");
      rotateFull(2);
    }
    rotateDial(position, 2);
  }
}

/**
 * Debug output to print the current state of all disks
*/
void printAllDiskState() {
  printDiskState(2);
  printDiskState(1);
  printDiskState(0);
}

void printDiskState(uint8 disk) {
  Serial.print("Disk ");
  Serial.print(disk);
  Serial.print(" currently at position ");
  if (currentDiskPosition[disk] == 255) {
    Serial.print("<unknown>");
  } else {
    Serial.print(currentDiskPosition[disk]);
  }
  Serial.print(" with rotation Mode ");
  if (rotationMode[disk] == 0) {
    Serial.print("<unknown>");
  } else if (rotationMode[disk] == 1) {
    Serial.print("clockwise");
  } else {
    Serial.print("counter-clockwise");
  }
  Serial.println(".");
}
