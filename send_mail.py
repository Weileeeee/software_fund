import sys
import smtplib
from email.mime.text import MIMEText
from email.mime.multipart import MIMEMultipart

# ============================================================
# ⚙️ CONFIGURATION (YOU MUST CHANGE THIS)
# ============================================================
# 1. Put YOUR Gmail address here
SENDER_EMAIL = "soh20040101@gmail.com"  
SENDER_PASSWORD = "zdwz jjej iqli pbla"

SMTP_SERVER = "smtp.gmail.com"
SMTP_PORT = 587
# ============================================================

def send_broadcast_notification(email_list_str, incident_type, location):
    recipients = [e.strip() for e in email_list_str.split(',') if e.strip()]
    
    if not recipients:
        print("[Python] No recipients found.")
        return

    print(f"[Python] BROADCASTING to {len(recipients)} students...")
    
    subject = f"⚠️ CAMPUS ALERT: {incident_type} at {location}"
    body = f"""
    URGENT CAMPUS ALERT
    ===================
    TYPE:     {incident_type}
    LOCATION: {location}
    STATUS:   ACTIVE - PLEASE AVOID THE AREA
    
    Stay Safe,
    UniSafe 360 Security Team
    """

    try:
        # Connect to Gmail
        server = smtplib.SMTP(SMTP_SERVER, SMTP_PORT)
        server.starttls() 
        
        # Login
        server.login(SENDER_EMAIL, SENDER_PASSWORD)

        # Send Emails
        count = 0
        for student_email in recipients:
            msg = MIMEMultipart()
            msg['From'] = SENDER_EMAIL
            msg['To'] = student_email
            msg['Subject'] = subject
            msg.attach(MIMEText(body, 'plain'))
            
            try:
                server.sendmail(SENDER_EMAIL, student_email, msg.as_string())
                count += 1
                print(f"[Python] Sent to: {student_email}")
            except Exception as inner_e:
                print(f"[Python] Failed to send to {student_email}: {str(inner_e)}")

        server.quit()
        print(f"[Python] ✅ Broadcast Complete: {count}/{len(recipients)} sent.")
        
    except smtplib.SMTPAuthenticationError:
        print("\n[Python] ❌ AUTHENTICATION ERROR!")
        print("1. Check if SENDER_EMAIL is correct.")
        print("2. Check if SENDER_PASSWORD is a valid 16-char App Password.")
        print("3. Ensure 2-Step Verification is ON in your Google Account.\n")
    except Exception as e:
        print(f"[Python] ❌ GENERAL ERROR: {str(e)}")

if __name__ == "__main__":
    print(f"[Python] Script started with {len(sys.argv)} arguments.")
    if len(sys.argv) > 3:
        target_emails = sys.argv[1]
        inc_type = sys.argv[2]
        inc_loc = sys.argv[3]
        print(f"[Python] Target Emails: {target_emails}")
        print(f"[Python] Incident Type: {inc_type}")
        print(f"[Python] Location: {inc_loc}")
        send_broadcast_notification(target_emails, inc_type, inc_loc)
    else:
        print("[Python] Args missing.")
        for i, arg in enumerate(sys.argv):
            print(f"  Arg {i}: {arg}")