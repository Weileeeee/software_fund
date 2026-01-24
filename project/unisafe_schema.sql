-- Full SQL Schema based on 5 ERD Images

SET FOREIGN_KEY_CHECKS = 0;

-- 1. Users
CREATE TABLE IF NOT EXISTS users (
    user_id INT AUTO_INCREMENT PRIMARY KEY,
    username VARCHAR(50) UNIQUE,
    password_hash VARCHAR(255),
    full_name VARCHAR(100),
    email VARCHAR(100) UNIQUE,
    role ENUM('Student', 'Admin', 'Warden', 'Security', 'Lecturer'),
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    student_code VARCHAR(20),
    is_hostel_resident TINYINT(1) DEFAULT 0,
    block VARCHAR(10),
    room_number VARCHAR(10),
    stay_status VARCHAR(50),
    whatsapp_number VARCHAR(20)
);

-- 2. Student Profiles
CREATE TABLE IF NOT EXISTS studentprofiles (
    profile_id INT AUTO_INCREMENT PRIMARY KEY,
    user_id INT,
    student_code VARCHAR(20),
    residence_block VARCHAR(50),
    room_number VARCHAR(20),
    class_group VARCHAR(50),
    residency_status ENUM('Resident', 'Non-Resident', 'Day Scholar'),
    safety_status ENUM('Safe', 'Missing', 'Injured', 'Unknown'),
    last_status_updated_by INT,
    last_status_update_time TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    FOREIGN KEY (user_id) REFERENCES users(user_id) ON DELETE CASCADE
);

-- 3. Incidents
CREATE TABLE IF NOT EXISTS incidents (
    incident_id INT AUTO_INCREMENT PRIMARY KEY,
    reporter_name VARCHAR(100), 
    incident_type VARCHAR(100),
    location_name VARCHAR(100),
    reported_at DATETIME,
    incident_status VARCHAR(50),
    latitude DOUBLE,
    longitude DOUBLE,
    description VARCHAR(255)
);

-- 4. Warden Reports (Linked to Incidents)
CREATE TABLE IF NOT EXISTS wardenreports (
    report_id INT AUTO_INCREMENT PRIMARY KEY,
    incident_id INT,
    warden_name VARCHAR(100),
    report_details TEXT,
    submitted_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (incident_id) REFERENCES incidents(incident_id) ON DELETE CASCADE
);

-- 5. Incident Media
CREATE TABLE IF NOT EXISTS incidentmedia (
    media_id INT AUTO_INCREMENT PRIMARY KEY,
    incident_id INT,
    file_path VARCHAR(255),
    uploaded_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (incident_id) REFERENCES incidents(incident_id) ON DELETE CASCADE
);

-- 6. Emergency Alerts
CREATE TABLE IF NOT EXISTS emergencyalerts (
    alert_id INT AUTO_INCREMENT PRIMARY KEY,
    sender_id INT,
    title VARCHAR(100),
    message_content TEXT,
    alert_type VARCHAR(50),
    sent_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (sender_id) REFERENCES users(user_id) ON DELETE SET NULL
);

-- 7. Alert Delivery
CREATE TABLE IF NOT EXISTS alertdelivery (
    delivery_id INT AUTO_INCREMENT PRIMARY KEY,
    alert_id INT,
    recipient_user_id INT,
    channel ENUM('Email', 'SMS', 'App', 'Push'),
    delivery_status ENUM('Sent', 'Failed', 'Delivered'),
    acknowledged TINYINT(1) DEFAULT 0,
    FOREIGN KEY (alert_id) REFERENCES emergencyalerts(alert_id) ON DELETE CASCADE,
    FOREIGN KEY (recipient_user_id) REFERENCES users(user_id) ON DELETE CASCADE
);

-- 8. Emergency Alert Preferences
CREATE TABLE IF NOT EXISTS emergencyalertpreferences (
    id INT AUTO_INCREMENT PRIMARY KEY,
    user_id INT,
    email_enabled TINYINT(1) DEFAULT 1,
    whatsapp_enabled TINYINT(1) DEFAULT 0,
    FOREIGN KEY (user_id) REFERENCES users(user_id) ON DELETE CASCADE
);

-- 9. Evacuation Alerts
CREATE TABLE IF NOT EXISTS evacuationalerts (
    alert_id INT AUTO_INCREMENT PRIMARY KEY,
    title VARCHAR(100),
    description TEXT,
    block VARCHAR(10),
    created_by INT,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    status ENUM('Active', 'Resolved', 'Cancelled') DEFAULT 'Active',
    FOREIGN KEY (created_by) REFERENCES users(user_id) ON DELETE SET NULL
);

-- 10. Evacuation Logs (Tracking User Status during Evacuation)
CREATE TABLE IF NOT EXISTS evacuationlogs (
    log_id INT AUTO_INCREMENT PRIMARY KEY,
    alert_id INT,
    user_id INT,
    status ENUM('Safe', 'Missing', 'Unknown') DEFAULT 'Unknown',
    last_updated TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    FOREIGN KEY (alert_id) REFERENCES evacuationalerts(alert_id) ON DELETE SET NULL,
    FOREIGN KEY (user_id) REFERENCES users(user_id) ON DELETE CASCADE
);

-- 11. Evacuation Attendance (Specific Record keeping)
CREATE TABLE IF NOT EXISTS evacuationattendance (
    record_id INT AUTO_INCREMENT PRIMARY KEY,
    incident_id INT, 
    student_id INT,
    status ENUM('Present', 'Missing', 'Safe', 'Unknown') DEFAULT 'Unknown',
    check_in_time TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (student_id) REFERENCES users(user_id) ON DELETE CASCADE
    -- Note: incident_id refers to 'incidents' or 'evacuationalerts'? Diagram links to 'evacuationlogs' which links to 'users'. 
    -- Assuming independent use or linked to incidents table if needed.
);

-- 12. Lecturer Instructions
CREATE TABLE IF NOT EXISTS lecturerinstructions (
    instruction_id INT AUTO_INCREMENT PRIMARY KEY,
    lecturer_id INT,
    target_class_group VARCHAR(50),
    instruction_content TEXT,
    sent_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (lecturer_id) REFERENCES users(user_id) ON DELETE CASCADE
);

-- 13. Notifications (General)
CREATE TABLE IF NOT EXISTS notifications (
    id INT AUTO_INCREMENT PRIMARY KEY,
    user_id INT,
    channel ENUM('Email', 'SMS', 'Push'),
    status ENUM('Sent', 'Read', 'Failed'),
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (user_id) REFERENCES users(user_id) ON DELETE CASCADE
);

-- 14. Alerts (Simple Table from Image 3)
CREATE TABLE IF NOT EXISTS alerts (
    alert_id INT AUTO_INCREMENT PRIMARY KEY,
    title VARCHAR(255),
    message TEXT,
    block VARCHAR(50),
    status ENUM('Active', 'Inactive'),
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP
);

SET FOREIGN_KEY_CHECKS = 1;
