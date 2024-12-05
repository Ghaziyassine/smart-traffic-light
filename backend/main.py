from ultralytics import YOLO
import cv2
import numpy as np

def detect_vehicles(video_paths):
    # Load the YOLOv8 model
    model = YOLO('yolov8n.pt')

    # Open video captures
    caps = [cv2.VideoCapture(video_path) for video_path in video_paths]

    # Set the desired frame width and height (reduce resolution)
    frame_width = 320
    frame_height = 240

    while True:
        frames = []
        for cap in caps:
            # Read a frame from the video
            success, frame = cap.read()
            if success:
                # Resize the frame to reduce resolution
                frame = cv2.resize(frame, (frame_width, frame_height))
                frames.append(frame)
            else:
                frames.append(np.zeros((frame_height, frame_width, 3), dtype=np.uint8))

        # Concatenate frames into a single large frame
        top_row = np.hstack((frames[0], frames[1]))
        bottom_row = np.hstack((frames[2], frames[3]))
        large_frame = np.vstack((top_row, bottom_row))

        # Split the large frame into four parts
        sub_frames = [
            large_frame[0:frame_height, 0:frame_width],
            large_frame[0:frame_height, frame_width:frame_width*2],
            large_frame[frame_height:frame_height*2, 0:frame_width],
            large_frame[frame_height:frame_height*2, frame_width:frame_width*2]
        ]

        for i, sub_frame in enumerate(sub_frames):
            # Perform detection
            results = model(sub_frame, conf=0.25, classes=[2, 3, 5, 7])  # Filter for car, motorcycle, bus, truck

            # Count the number of cars
            car_count = sum(1 for result in results for box in result.boxes if int(box.cls[0]) == 2)

            # Draw bounding boxes on the frame
            for result in results:
                for box in result.boxes:
                    # Get box coordinates
                    x1, y1, x2, y2 = map(int, box.xyxy[0])
                    conf = box.conf[0]
                    cls = int(box.cls[0])
                    label = f'{model.names[cls]} {conf:.2f}'
                    cv2.rectangle(sub_frame, (x1, y1), (x2, y2), (0, 255, 0), 2)
                    cv2.putText(sub_frame, label, (x1, y1 - 10), cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0, 255, 0), 2)

            # Display the frame with car count
            cv2.putText(sub_frame, f'Car Count: {car_count}', (10, 30), cv2.FONT_HERSHEY_SIMPLEX, 1, (0, 0, 255), 2)
            cv2.imshow(f'Video {i+1}', sub_frame)

        # Break the loop if 'q' is pressed
        if cv2.waitKey(1) & 0xFF == ord('q'):
            break

    # Release the video captures and close windows
    for cap in caps:
        cap.release()
    cv2.destroyAllWindows()

# List of video paths
video_paths = ["video/traffic1.mp4", "video/traffic2.mp4", "video/traffic3.mp4", "video/traffic4.mp4"]

# Call the function
detect_vehicles(video_paths)