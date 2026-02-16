# Architecture du Système ROD (Remote Observation Device)

## Vue d'Ensemble

ROD est un système de vision par ordinateur pour détecter des marqueurs ArUco sur des éléments de jeu Eurobot et transmettre leurs positions aux robots via socket Unix.

## Architecture Modulaire

```
rod_c/
├── rod_detection.c          # Thread principal (CV + orchestration)
├── rod_communication.c      # Thread IPC (réception données)
│
├── rod_config/              # Configuration centralisée
│   ├── IDs valides Eurobot 2026
│   ├── Paramètres détecteur ArUco optimisés
│   └── Constantes système (chemins, intervalles)
│
├── rod_cv/                  # Vision par ordinateur
│   ├── Calculs géométriques (centre, angle, périmètre)
│   ├── Filtrage des marqueurs valides
│   ├── Comptage par catégorie
│   └── Types standards (MarkerData, MarkerCounts)
│
├── rod_visualization/       # Visualisation & Debug
│   ├── Annotation avec IDs
│   ├── Annotation avec centres
│   ├── Annotation avec compteurs
│   └── Sauvegarde images debug
│
├── rod_socket/              # Communication inter-processus
│   ├── Serveur socket Unix domain
│   ├── Gestion connexions clients
│   └── Envoi données de détection (JSON-like)
│
├── rod_camera/              # Abstraction caméra
│   ├── emulated_camera (test)
│   └── libcamera (production)
│
└── opencv_wrapper/          # Interface C vers OpenCV
    └── Wrapper C++ → C pour détection ArUco
```

## Flux de Données

```
┌──────────────────────────────────────────────────────────────┐
│                    rod_detection (Thread CV)                 │
│                                                              │
│  1. Caméra → Capture image                                   │
│  2. ArUco  → Détection (opencv_wrapper)                      │
│  3. rod_cv → Filtrage IDs valides                            │
│  4. rod_socket → Envoi détections                            │
│  5. rod_visualization → Sauvegarde debug                     │
└──────────────────────┬───────────────────────────────────────┘
                       │
                       │ Unix Socket: /tmp/rod_detection.sock
                       │ Format: [[id,x,y,angle], ...]
                       │
                       ▼
┌─────────────────────────────────────────────────────────────┐
│              rod_communication (Thread IPC)                 │
│                                                             │
│  1. Socket client → Connexion                               │
│  2. Réception → Données détection                           │
│  3. Affichage → Console (debug)                             │
│  4. TODO: Transmission → Robot principal                    │
└─────────────────────────────────────────────────────────────┘
```

## Modules et Responsabilités

### rod_config - Configuration
**Rôle** : Point unique de configuration  
**Exports** :
- `rod_config_is_valid_marker_id()` - Validation IDs Eurobot
- `rod_config_configure_detector_parameters()` - Paramètres ArUco
- Macros : `ROD_SOCKET_PATH`, `ROD_DEBUG_OUTPUT_FOLDER`, etc.



### rod_cv - Vision par Ordinateur
**Rôle** : Opérations géométriques et traitement détections  
**Exports** :
- `calculate_marker_center()`, `calculate_marker_angle()`
- `filter_valid_markers()` - Filtrage + conversion DetectionResult → MarkerData[]
- `count_markers_by_category()` - Comptage par type
- Types : `MarkerData`, `MarkerCounts`, `Point2f`, `Pose2D/3D`



### rod_visualization - Visualisation
**Rôle** : Annotations et debug visuel  
**Exports** :
- `rod_viz_annotate_with_ids()` - Affiche IDs
- `rod_viz_annotate_with_centers()` - Affiche coordonnées
- `rod_viz_annotate_with_counter()` - Affiche compteurs
- `rod_viz_save_debug_image()` - Sauvegarde complète annotée



### rod_socket - Communication
**Rôle** : Encapsulation socket Unix domain  
**Exports** :
- `rod_socket_server_create()` - Création serveur
- `rod_socket_server_accept()` - Acceptation client (non-bloquant)
- `rod_socket_server_send_detections()` - Envoi JSON-like
- `rod_socket_server_destroy()` - Nettoyage


### rod_camera - Abstraction Caméra
**Rôle** : Interface unifiée caméras  
**Implémentations** :
- `emulated_camera` - Lecture images dossier (test)
- `libcamera` - Caméra Raspberry Pi (production)


### opencv_wrapper - Bridge C/C++
**Rôle** : Interface C vers OpenCV C++  
**Pattern** : Handles opaques + wrappers fonctions  
**Exports** : `detectMarkersWithConfidence()`, `sharpen_image()`, etc.
