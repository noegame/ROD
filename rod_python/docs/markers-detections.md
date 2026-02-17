## My functions

**Markers detection**
Find the markers in the input image and return their corners coordinates in the image and IDs.
```plantuml
@startuml
left to right direction
rectangle {
    [aruco detector] --> [detect_markers]
    [Img] --> [detect_markers]
    [mask]  --> [detect_markers]
    [detect_markers] --> [corners (matrice Nx4)]
    [detect_markers] --> [IDs (matrice Nx1)]
    [detect_markers] --> [rejected]
}
@enduml
```
**Get Aruco Detector**
Create and return an Aruco marker detector with specified parameters.
```plantuml
@startuml
left to right direction
rectangle {
    [get_aruco_detector] --> [aruco detector]
}
@enduml
```

**Coordinate conversion**
convert the coordinates of the image coordinate system to the terrain coordinate system
```plantuml
@startuml
left to right direction
rectangle {
    [corners (matrice Nx4)] --> [pose_estimation]
    [convert_coord] --> [new_corners (matrice Nx4)]
}
@enduml
```

**Centers calculation**
Find the center coordinates of each detected marker from their corners coordinates
```plantuml
@startuml
left to right direction
rectangle {
    [corners (matrice Nx4)] --> [find_center_coord]
    [find_center_coord] --> [Centers (matrice Nx1)]
}
@enduml
```

**Detection statistics**
Print statistics about the detected markers
```plantuml
@startuml
left to right direction
rectangle {
    [IDs (matrice Nx1)] --> [print_detection_stats]
    [Centers (matrice Nx1)] --> [print_detection_stats]
    [print_detection_stats] --> [None]
}
@enduml
```

**Find mask**
Create a mask that hides everything except the playground area
```plantuml
@startuml
left to right direction
rectangle {
    [Img] --> [find_mask]
    [find_mask] --> [mask]
}
@enduml
```

**Annotate image**
(optionnal) Draw the ID of detected markers on the input image
(optionnal) Draw the markers boundaries on the input image
(optionnal) Draw the markers axes on the input image
(optionnal) Draw the markers corners on the input image
(optionnal) Draw the markers coords on the input image
```plantuml
@startuml
left to right direction
rectangle {
    [Img] --> [draw_markers]
    [corners img coord (matrice Nx4)] --> [draw_markers]
    [corners real world coord (matrice Nx4)] --> [draw_markers]
    [IDs (matrice Nx1)] --> [draw_markers]
    [draw_markers] --> [Img with markers]
}
@enduml
```
