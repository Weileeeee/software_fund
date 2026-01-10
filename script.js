document.addEventListener('DOMContentLoaded', () => {
    const sections = document.querySelectorAll('.tab-content');
    const navLinks = document.querySelectorAll('.nav-link');
    const logoutBtn = document.getElementById('logoutBtn');
    
    // 获取表格和表单元素
    const pendingTbody = document.getElementById('pending-tbody');
    const activeTbody = document.getElementById('active-tbody');
    const incidentForm = document.getElementById('incidentForm');
    
    let currentUserRole = ""; 
    let map;

    // --- 1. 导航切换逻辑 ---
    function showSection(id) {
        sections.forEach(s => {
            s.classList.remove('active');
            s.style.display = "none";
        });
        const target = document.getElementById(id);
        if (target) {
            target.classList.add('active');
            target.style.display = (id === 'login-section') ? "flex" : "block";
            if (id === 'incidents-section' && map) {
                setTimeout(() => map.invalidateSize(), 200);
            }
        }
    }

    navLinks.forEach(link => {
        link.addEventListener('click', function(e) {
            e.preventDefault();
            navLinks.forEach(l => l.classList.remove('active'));
            this.classList.add('active');
            showSection(this.getAttribute('data-target'));
        });
    });

    // --- 2. 登录角色处理 ---
    document.getElementById('goStudentBtn').onclick = () => {
        currentUserRole = "student";
        document.getElementById('loginTitle').innerText = "Student Login";
        document.getElementById('captchaArea').style.display = "block";
        showSection('login-section');
    };

    document.getElementById('goSecurityBtn').onclick = () => {
        currentUserRole = "security";
        document.getElementById('loginTitle').innerText = "Security Login";
        document.getElementById('captchaArea').style.display = "none";
        showSection('login-section');
    };

    document.getElementById('loginForm').onsubmit = (e) => {
        e.preventDefault();
        // 简单模拟登录
        document.getElementById('security-panel').style.display = (currentUserRole === "security") ? "block" : "none";
        document.getElementById('openModalBtn').style.display = (currentUserRole === "security") ? "none" : "block";
        logoutBtn.style.display = "block";
        showSection('incidents-section');
        initMap();
    };

    // --- 3. 学生报案提交逻辑 ---
    incidentForm.onsubmit = (e) => {
        e.preventDefault();
        
        // 获取输入值
        const type = document.getElementById('inType').value;
        const loc = document.getElementById('inLoc').value;
        const time = document.getElementById('inTime').value;
        const status = "Pending Approval"; // 默认状态

        // 创建一行显示在保安的“待审批”表格中
        const rowId = Date.now(); // 唯一ID用于识别
        const tr = document.createElement('tr');
        tr.setAttribute('data-id', rowId);
        tr.innerHTML = `
            <td>${type}</td>
            <td>${loc}</td>
            <td>${time}</td>
            <td>
                <button class="approve-btn" onclick="handleReport(${rowId}, 'approve')">Approve</button>
                <button class="delete-btn" onclick="handleReport(${rowId}, 'reject')" style="background:#e74c3c; color:white; border:none; padding:7px; border-radius:6px; margin-left:5px; cursor:pointer;">Not Approve</button>
            </td>
        `;
        pendingTbody.appendChild(tr);

        // 显示成功提示动画
        document.getElementById('successOverlay').style.display = "flex";
        setTimeout(() => {
            document.getElementById('successOverlay').style.display = "none";
            document.getElementById('reportModal').style.display = "none";
            incidentForm.reset(); // 重置表单
        }, 1500);
    };

    // --- 4. 保安审批/拒绝逻辑 ---
    window.handleReport = function(id, action) {
        const row = document.querySelector(`tr[data-id="${id}"]`);
        if (!row) return;

        if (action === 'approve') {
            const cells = row.querySelectorAll('td');
            const type = cells[0].innerText;
            const loc = cells[1].innerText;
            const time = cells[2].innerText;

            // 移动到同步的活跃表格中 (Active Incidents)
            const activeTr = document.createElement('tr');
            activeTr.innerHTML = `
                <td>${type}</td>
                <td>${loc}</td>
                <td>${time}</td>
                <td><span class="type-tag approved">Approved</span></td>
            `;
            activeTbody.appendChild(activeTr);

            // 在地图上标记
            if (map) {
                L.marker([2.9270, 101.6410]).addTo(map)
                    .bindPopup(`<b>${type}</b><br>${loc}`).openPopup();
            }
            alert("Report Approved and Updated for everyone!");
        } else {
            alert("Report Not Approved. It has been deleted.");
        }
        
        // 无论批准还是拒绝，都从待办列表中移除
        row.remove();
    };

    // --- 5. 弹窗控制 ---
    document.getElementById('openModalBtn').onclick = () => {
        document.getElementById('reportModal').style.display = "flex";
    };
    document.querySelector('.close-btn').onclick = () => {
        document.getElementById('reportModal').style.display = "none";
    };

    // --- 6. 地图初始化 ---
    function initMap() {
        if (!map) {
            map = L.map('map').setView([2.9276, 101.6413], 17);
            L.tileLayer('https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png').addTo(map);
        }
    }

    logoutBtn.onclick = () => location.reload();
});