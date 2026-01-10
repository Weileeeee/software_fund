import sys
import smtplib
from email.mime.text import MIMEText
from email.mime.multipart import MIMEMultipart

# ============================================================
# ⚙️ 配置区域 (请修改这里)
# ============================================================
# 发件人: 强烈建议使用 Gmail (稳定性最高)
SENDER_EMAIL = "soh20040101@gmail.com"  
# 密码: 必须是 Google 账户设置里生成的 16 位 "App Password"
SENDER_PASSWORD = "zdwz jjej iqli pbla"        

# SMTP 服务器 (Gmail)
SMTP_SERVER = "smtp.gmail.com"
SMTP_PORT = 587
# ============================================================

def send_broadcast_notification(email_list_str, incident_type, location):
    # 1. 处理邮箱列表：按逗号拆分，去除空格，过滤空值
    recipients = [e.strip() for e in email_list_str.split(',') if e.strip()]
    
    if not recipients:
        print("[Python] No recipients found to send email to.")
        return

    print(f"\n[Python] Starting BROADCAST to {len(recipients)} students...")
    print(f"[Python] Recipients: {recipients}")

    subject = f"⚠️ CAMPUS ALERT: {incident_type} at {location}"
    
    body = f"""
    URGENT CAMPUS ALERT
    ===================
    
    Security has confirmed an incident on campus.
    
    TYPE:     {incident_type}
    LOCATION: {location}
    STATUS:   ACTIVE - PLEASE AVOID THE AREA
    
    Security team has been dispatched.
    
    Stay Safe,
    UniSafe 360 Security Team
    """

    try:
        # 连接 Gmail 服务器
        server = smtplib.SMTP(SMTP_SERVER, SMTP_PORT)
        server.starttls() # 开启加密
        server.login(SENDER_EMAIL, SENDER_PASSWORD)

        # 2. 循环发送给每一个人
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
        print(f"[Python] ✅ Broadcast Complete. Successfully sent {count}/{len(recipients)} emails.")
        
    except Exception as e:
        print(f"[Python] ❌ FAILED: {str(e)}")
        print("[Python] Hint: Check your 'App Password' or internet connection.")

if __name__ == "__main__":
    # 接收 C++ 传来的参数
    # 参数1: 邮箱列表(逗号隔开)  参数2: 类型  参数3: 地点
    if len(sys.argv) > 3:
        target_emails = sys.argv[1]
        inc_type = sys.argv[2]
        inc_loc = sys.argv[3]
        send_broadcast_notification(target_emails, inc_type, inc_loc)
    else:
        print("[Python] No arguments provided. Usage: python send_mail.py \"email1,email2\" \"Type\" \"Loc\"")