from ultralytics import YOLO
import cv2
import numpy as np

def detect_vehicles():
    # Load the YOLOv8 model
    model = YOLO('yolov8n.pt')

    # Open video capture
    cap = cv2.VideoCapture("video/traffic.mp4")

    while cap.isOpened():
        # Read a frame from the video
        success, frame = cap.read()

        if success:
            # Perform detection
            results = model(frame, conf=0.25, classes=[2, 3, 5, 7])  # Filter for car, motorcycle, bus, truck

            # Draw bounding boxes on the frame
            for result in results:
                for box in result.boxes:
                    # Get box coordinates
                    x1, y1, x2, y2 = map(int, box.xyxy[0])
                    conf = box.conf[0]
                    cls = int(box.cls[0])
                    label = f'{model.names[cls]} {conf:.2f}'
                    cv2.rectangle(frame, (x1, y1), (x2, y2), (0, 255, 0), 2)
                    cv2.putText(frame, label, (x1, y1 - 10), cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0, 255, 0), 2)

            # Display the frame
            cv2.imshow('Vehicle Detection', frame)

            # Break the loop if 'q' is pressed
            if cv2.waitKey(1) & 0xFF == ord('q'):
                break
        else:
            break

    # Release the video capture and close windows
    cap.release()
    cv2.destroyAllWindows()

# Call the function
detect_vehicles()