# ROD - Remote Observation Device

## What is ROD

As part of the [Eurobot](https://www.eurobot.org/) competition and the [French Robotics Cup](https://www.coupederobotique.fr/), robots compete against each other in matches on a game table. The competition rules allow the use of a Remote Observation Device (ROD): a system equipped with a camera that captures images of the game table and transmits information to the robot.

The objectives of this ROD project are:
- Recognition of ArUco TAGs on game elements using a camera mounted above the table
- Ability to precisely locate game elements on the playing field
- Transmission of game element positions to a robot

## How it works

ROD is working on two threads that communicate with each other through socket communication. The first thread is responsible for the computer vision part, while the second thread is responsible for the communication part.

The computer vision thread captures images from the camera, processes them to detect ArUco TAGs, and calculates the coordinates of the detected objects. 

The communication thread is responsible for printing the detected objects coordinates in the console. In futur it will be responsible for sending the detected objects coordinates to the main process of the robot.

The computer vision thread send to the communication thread via socket an array that contain the list of detected objects with their coordinates [[id, x,y,angle], [id, x,y,angle], ...].



```plantuml
rectangle "Processus ROD" {
    rectangle "Process Computer Vision" {
        rectangle "Take picture" as Take_picture {
        }
        rectangle "Detect ArUco TAGs" as Detect_ArUco_TAGs {
        }
        rectangle "Calculate coordinates" as Calculate_coordinates {
        }
        rectangle "Send coordinates to communication thread via socket" as socket{
        }
        Take_picture            --> Detect_ArUco_TAGs
        Detect_ArUco_TAGs       --> Calculate_coordinates
        Calculate_coordinates   --> socket
    }
    rectangle "Process Communication" {
        rectangle "Receive coordinates from computer vision thread via socket" as receive_socket {
        }
        rectangle "Print coordinates in console" as print_coordinates {
        }
        receive_socket         --> print_coordinates
    }
}
```

## How to use
- [How to build](docs/how-to-build.md)
- [How to run](docs/how-to-run.md)
