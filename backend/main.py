from ultralytics import YOLO
import cv2
import numpy as np
from flask import Flask, jsonify
import threading
import sys

app = Flask(__name__)

# Load the YOLOv8 model
model = YOLO('yolov8n.pt')

# Set the desired frame width and height
frame_width = 320
frame_height = 240

vehicle_counts = [0, 0, 0, 0]
stop_flag = False
processing_thread = None

def resize_frame(frame):
    return cv2.resize(frame, (frame_width, frame_height))

def cleanup_resources(caps):
    for cap in caps:
        cap.release()
    cv2.destroyAllWindows()

def detect_vehicles(video_paths):
    global vehicle_counts, stop_flag
    caps = []
    
    try:
        # Initialize video captures
        caps = [cv2.VideoCapture(path) for path in video_paths]
        if not all(cap.isOpened() for cap in caps):
            raise Exception("Error: Couldn't open one or more video streams")

        while not stop_flag:
            # Read and check frames
            frames = []
            for cap in caps:
                ret, frame = cap.read()
                if not ret:
                    stop_flag = True
                    break
                frames.append(resize_frame(frame))

            if stop_flag:
                break

            # Process frames
            top_row = np.hstack((frames[0], frames[1]))
            bottom_row = np.hstack((frames[2], frames[3]))
            large_frame = np.vstack((top_row, bottom_row))

            sub_frames = [
                large_frame[0:frame_height, 0:frame_width],
                large_frame[0:frame_height, frame_width:frame_width*2],
                large_frame[frame_height:frame_height*2, 0:frame_width],
                large_frame[frame_height:frame_height*2, frame_width:frame_width*2]
            ]

            for i, sub_frame in enumerate(sub_frames):
                results = model(sub_frame, conf=0.25, classes=[2, 3, 5, 7])
                vehicle_counts[i] = sum(1 for result in results for box in result.boxes if int(box.cls[0]) in [2, 3, 5, 7])

                # Draw results
                for result in results:
                    for box in result.boxes:
                        x1, y1, x2, y2 = map(int, box.xyxy[0])
                        cv2.rectangle(sub_frame, (x1, y1), (x2, y2), (0, 255, 0), 2)

                cv2.putText(sub_frame, f'Count: {vehicle_counts[i]}', (10, 30), 
                           cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0, 0, 255), 2)
                cv2.imshow(f'Video {i+1}', sub_frame)

            # Check for quit command
            if cv2.waitKey(1) & 0xFF == ord('q'):
                stop_flag = True
                break

    except Exception as e:
        print(f"Error: {str(e)}")
        stop_flag = True
    
    finally:
        cleanup_resources(caps)

@app.route('/stop', methods=['GET'])
def stop_processing():
    global stop_flag
    stop_flag = True
    return jsonify({'status': 'stopping'})

@app.route('/traffic<int:id>', methods=['GET'])
def get_traffic(id):
    if 1 <= id <= 4:
        return jsonify({'vehicle_count': vehicle_counts[id-1]})
    return jsonify({'error': 'Invalid traffic ID'}), 400

def start_flask():
    app.run(host='0.0.0.0', port=5000)

if __name__ == '__main__':
    try:
        video_paths = ["video/traffic1.mp4", "video/traffic2.mp4", 
                      "video/traffic3.mp4", "video/traffic4.mp4"]
        
        # Start Flask in a separate thread
        flask_thread = threading.Thread(target=start_flask, daemon=True)
        flask_thread.start()

        # Start video processing
        processing_thread = threading.Thread(target=detect_vehicles, 
                                          args=(video_paths,), daemon=True)
        processing_thread.start()

        # Wait for processing to complete
        processing_thread.join()

    except KeyboardInterrupt:
        print("\nShutting down...")
        stop_flag = True
        sys.exit(0)