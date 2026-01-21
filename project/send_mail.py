import sys
import smtplib
from email.mime.text import MIMEText
from email.mime.multipart import MIMEMultipart

# ============================================================
# ⚙️ CONFIG (Update these)
# ============================================================
SENDER_EMAIL = "leongweilee49@gmail.com"  
SENDER_PASSWORD = "gjmp brcv xziv ggxp"        

SMTP_SERVER = "smtp.gmail.com"
SMTP_PORT = 587
# ============================================================

def send_broadcast_notification(email_list_str, incident_type, location):
    recipients = [e.strip() for e in email_list_str.split(',') if e.strip()]
    
    if not recipients:
        print("[Python] No recipients found.")
        return

    print(f"\n[Python] BROADCASTING to {len(recipients)} students...")
    subject = f"⚠️ CAMPUS ALERT: {incident_type} at {location}"
    
    body = f"""
    URGENT CAMPUS ALERT
    ===================
    
    Security has confirmed an incident on campus.
    
    TYPE:     {incident_type}
    LOCATION: {location}
    STATUS:   ACTIVE - PLEASE AVOID THE AREA
    
    Stay Safe,
    UniSafe 360 Security Team
    """

    try:
        server = smtplib.SMTP(SMTP_SERVER, SMTP_PORT)
        server.starttls() 
        server.login(SENDER_EMAIL, SENDER_PASSWORD)

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
        
    except Exception as e:
        print(f"[Python] ❌ FAILED: {str(e)}")

if __name__ == "__main__":
    if len(sys.argv) > 3:
        target_emails = sys.argv[1]
        inc_type = sys.argv[2]
        inc_loc = sys.argv[3]
        send_broadcast_notification(target_emails, inc_type, inc_loc)
    else:
        print("[Python] Args missing.")