#!/usr/bin/env python3
"""
Create timelapse video from timestamped photos.

Takes a folder of photos with names in format YYYYMMDD_HHMMSS_XXX.jpg or .png
and creates an MP4 video. Repeats frames as needed to maintain 24fps
based on actual time intervals between photos.
"""

import argparse
import cv2
import os
import sys
from datetime import datetime
from pathlib import Path
from typing import List, Tuple


def parse_timestamp_from_filename(filename: str) -> datetime:
    """
    Extract timestamp from filename format YYYYMMDD_HHMMSS_XXX.jpg
    
    Args:
        filename: Photo filename
        
    Returns:
        datetime object representing the photo timestamp
        
    Raises:
        ValueError: If filename doesn't match expected format
    """
    name = Path(filename).stem  # Remove extension
    parts = name.split('_')
    
    if len(parts) < 2:
        raise ValueError(f"Invalid filename format: {filename}")
    
    date_str = parts[0]  # YYYYMMDD
    time_str = parts[1]  # HHMMSS
    
    # Parse timestamp
    timestamp_str = f"{date_str}_{time_str}"
    return datetime.strptime(timestamp_str, "%Y%m%d_%H%M%S")


def get_sorted_photos(folder: Path) -> List[Tuple[Path, datetime]]:
    """
    Get all JPG and PNG photos from folder sorted by timestamp.
    
    Args:
        folder: Directory containing photos
        
    Returns:
        List of (photo_path, timestamp) tuples sorted by time
    """
    photos = []
    
    # Support both JPG and PNG formats
    for pattern in ["*.jpg", "*.png"]:
        for file in folder.glob(pattern):
            try:
                timestamp = parse_timestamp_from_filename(file.name)
                photos.append((file, timestamp))
            except ValueError as e:
                print(f"Warning: Skipping {file.name}: {e}", file=sys.stderr)
    
    # Sort by timestamp
    photos.sort(key=lambda x: x[1])
    return photos


def create_timelapse_video(
    folder: Path,
    output: Path,
    fps: int = 24,
    speed_multiplier: float = 1.0
) -> None:
    """
    Create timelapse video from photos, repeating frames based on timestamps.
    
    Args:
        folder: Directory containing timestamped photos
        output: Output video file path
        fps: Target frames per second (default: 24)
        speed_multiplier: Speed multiplier (1.0 = real-time, 2.0 = 2x faster)
    """
    # Get sorted photos
    photos = get_sorted_photos(folder)
    
    if not photos:
        print(f"Error: No valid photos found in {folder}", file=sys.stderr)
        sys.exit(1)
    
    print(f"Found {len(photos)} photos")
    print(f"Time range: {photos[0][1]} to {photos[-1][1]}")
    
    # Read first image to get dimensions
    first_image = cv2.imread(str(photos[0][0]))
    if first_image is None:
        print(f"Error: Cannot read image {photos[0][0]}", file=sys.stderr)
        sys.exit(1)
    
    height, width = first_image.shape[:2]
    
    # Initialize video writer with H.264 codec
    fourcc = cv2.VideoWriter_fourcc(*'avc1')
    video_writer = cv2.VideoWriter(str(output), fourcc, fps, (width, height))
    
    if not video_writer.isOpened():
        print("Error: Cannot create video writer", file=sys.stderr)
        sys.exit(1)
    
    print(f"Creating video: {width}x{height} @ {fps}fps")
    
    total_frames_written = 0
    
    for i, (photo_path, timestamp) in enumerate(photos):
        # Read image
        img = cv2.imread(str(photo_path))
        if img is None:
            print(f"Warning: Cannot read {photo_path}, skipping", file=sys.stderr)
            continue
        
        # Calculate how many frames to show this image
        if i < len(photos) - 1:
            # Time difference to next photo in seconds
            next_timestamp = photos[i + 1][1]
            time_diff = (next_timestamp - timestamp).total_seconds()
            
            # Apply speed multiplier
            time_diff /= speed_multiplier
            
            # Number of frames = time_diff * fps
            num_frames = max(1, int(round(time_diff * fps)))
        else:
            # Last photo: show for 1 second
            num_frames = fps
        
        # Write frames
        print(f"  [{i+1}/{len(photos)}] {photo_path.name}: writing {num_frames} frames "
              f"({num_frames/fps:.2f}s)...", end='', flush=True)
        
        for _ in range(num_frames):
            video_writer.write(img)
            total_frames_written += 1
        
        print(" done")
    
    video_writer.release()
    
    duration = total_frames_written / fps
    print(f"\nVideo created: {output}")
    print(f"Total frames: {total_frames_written}")
    print(f"Duration: {duration:.2f} seconds ({duration/60:.2f} minutes)")


def main():
    parser = argparse.ArgumentParser(
        description="Create timelapse video from timestamped photos",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  %(prog)s photos/debug/ -o timelapse.mp4
  %(prog)s photos/debug/ -o timelapse.mp4 --fps 30 --speed 2.0
  
Photo filename format: YYYYMMDD_HHMMSS_XXX.jpg or YYYYMMDD_HHMMSS_XXX.png
        """
    )
    
    parser.add_argument(
        'folder',
        type=Path,
        help='Folder containing timestamped photos'
    )
    
    parser.add_argument(
        '-o', '--output',
        type=Path,
        default=Path('timelapse.mp4'),
        help='Output video file (default: timelapse.mp4)'
    )
    
    parser.add_argument(
        '--fps',
        type=int,
        default=24,
        help='Video frame rate (default: 24)'
    )
    
    parser.add_argument(
        '--speed',
        type=float,
        default=1.0,
        help='Speed multiplier (1.0 = real-time, 2.0 = 2x faster, default: 1.0)'
    )
    
    args = parser.parse_args()
    
    # Validate inputs
    if not args.folder.exists():
        print(f"Error: Folder not found: {args.folder}", file=sys.stderr)
        sys.exit(1)
    
    if not args.folder.is_dir():
        print(f"Error: Not a directory: {args.folder}", file=sys.stderr)
        sys.exit(1)
    
    if args.fps <= 0:
        print(f"Error: FPS must be positive", file=sys.stderr)
        sys.exit(1)
    
    if args.speed <= 0:
        print(f"Error: Speed multiplier must be positive", file=sys.stderr)
        sys.exit(1)
    
    # Create video
    create_timelapse_video(args.folder, args.output, args.fps, args.speed)


if __name__ == '__main__':
    main()
