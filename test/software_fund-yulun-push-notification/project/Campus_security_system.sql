-- 1. CREATE AND SELECT THE DATABASE
CREATE DATABASE IF NOT EXISTS CampusSecurityDB;
USE CampusSecurityDB;

-- 2. USERS TABLE
CREATE TABLE Users (
    user_id INT PRIMARY KEY AUTO_INCREMENT,
    username VARCHAR(50) UNIQUE NOT NULL,
    password_hash VARCHAR(255) NOT NULL,
    full_name VARCHAR(100) NOT NULL,
    email VARCHAR(100) NOT NULL,
    role ENUM('Student', 'Warden', 'Lecturer', 'SecurityOfficer', 'Admin') NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
    is_hostel_resident BOOLEAN DEFAULT FALSE;
    block VARCHAR(10) DEFAULT NULL;
);
USE CampusSecurityDB;

-- 1. Create the missing table
CREATE TABLE IF NOT EXISTS EvacuationLogs (
    log_id INT AUTO_INCREMENT PRIMARY KEY,
    user_id INT NOT NULL,
    status ENUM('Missing', 'Safe') DEFAULT 'Missing',
    last_updated TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    FOREIGN KEY (user_id) REFERENCES Users(user_id)
);

-- 2. Populate it with all students (Starts everyone as Safe)
TRUNCATE TABLE EvacuationLogs;

INSERT INTO EvacuationLogs (user_id, status) 
SELECT user_id, 'Safe' 
FROM Users 
WHERE role = 'Student';
-- 3. STUDENT PROFILES (Incorporating Safety Status & Assigned Staff)
-- Per Section 3.1.1: System records updates by staff



-- 4. INCIDENT MANAGEMENT (For the Interactive Map)
CREATE TABLE Incidents (
    incident_id INT AUTO_INCREMENT PRIMARY KEY,
    reporter_name VARCHAR(100),       -- Stores "John Doe"
    incident_type VARCHAR(100),       -- Stores "Fire", "Theft"
    location_name VARCHAR(100),       -- Stores "Block A"
    reported_at DATETIME,             -- Stores "2025-01-20 12:00:00"
    incident_status VARCHAR(50) DEFAULT 'Pending',
    latitude DOUBLE,
    longitude DOUBLE,
    description VARCHAR(255)
);

-- EVIDENCE TABLE (Warden optional uploads)
CREATE TABLE IncidentMedia (
    media_id INT PRIMARY KEY AUTO_INCREMENT,
    incident_id INT NOT NULL,
    file_path VARCHAR(255) NOT NULL,
    uploaded_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (incident_id) REFERENCES Incidents(incident_id) ON DELETE CASCADE
);

-- 5. EMERGENCY ALERTS
CREATE TABLE EmergencyAlerts (
    alert_id INT PRIMARY KEY AUTO_INCREMENT,
    sender_id INT NOT NULL, 
    title VARCHAR(100) NOT NULL,
    message_content TEXT NOT NULL,
    alert_type VARCHAR(50), 
    sent_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (sender_id) REFERENCES Users(user_id)
);

-- Multi-channel delivery tracking (Push/SMS/Email)
CREATE TABLE AlertDelivery (
    delivery_id INT PRIMARY KEY AUTO_INCREMENT,
    alert_id INT NOT NULL,
    recipient_user_id INT NOT NULL,
    channel ENUM('Push', 'SMS', 'Email') NOT NULL,
    delivery_status ENUM('Sent', 'Failed', 'Delivered') DEFAULT 'Sent',
    acknowledged BOOLEAN DEFAULT FALSE,
    FOREIGN KEY (alert_id) REFERENCES EmergencyAlerts(alert_id),
    FOREIGN KEY (recipient_user_id) REFERENCES Users(user_id)
);

-- 6. EVACUATION LOGS (Digital Roll Call)
CREATE TABLE EvacuationAttendance (
    record_id INT PRIMARY KEY AUTO_INCREMENT,
    incident_id INT NOT NULL, 
    student_id INT NOT NULL,
    status ENUM('Evacuated', 'Missing', 'Safe-SelfReported') NOT NULL, 
    check_in_time TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (incident_id) REFERENCES Incidents(incident_id),
    FOREIGN KEY (student_id) REFERENCES Users(user_id)
);

TRUNCATE TABLE EvacuationLogs;

INSERT INTO EvacuationLogs (user_id, status) 
SELECT user_id, 'Safe' 
FROM Users 
WHERE role = 'Student';
ALTER TABLE Users ADD COLUMN stay_status VARCHAR(50) DEFAULT 'Present';

ALTER TABLE Incidents 
ADD COLUMN description TEXT,
ADD COLUMN severity ENUM('Low', 'Medium', 'High', 'Critical') DEFAULT 'Medium',
ADD COLUMN evidence_path VARCHAR(255);