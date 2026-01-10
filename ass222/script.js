document.addEventListener('DOMContentLoaded', () => {
    
    // =========================================================
    // 🔐 自动身份认证 (Role: Admin vs Student)
    // =========================================================
    
    // 1. 获取 C++ 后端注入的角色信息
    const serverRole = window.SERVER_INJECTED_ROLE || 'student'; 
    
    // 2. 决定当前 Dashboard 的视图模式
    let currentUserRole = 'student'; 

    // 判断逻辑：数据库返回 'admin' 则为管理员，否则为学生
    if (serverRole.toLowerCase() === 'admin') {
        currentUserRole = 'security'; // 这里变量名保持 'security' 以兼容旧逻辑，但代表管理员
    } else {
        currentUserRole = 'student';
    }

    console.log(`Auto-Login View Mode: ${currentUserRole} (DB Role: ${serverRole})`);

    // =========================================================
    // 🚀 初始化 UI
    // =========================================================

    const sidebar = document.querySelector(".sidebar");
    const sidebarBtn = document.querySelector(".sidebarBtn");
    const navLinks = document.querySelectorAll(".nav-link");
    const tabContents = document.querySelectorAll(".tab-content");
    
    const securityPanel = document.getElementById("security-panel");
    const reportBtn = document.getElementById("openModalBtn");
    const logoutBtn = document.getElementById("logoutBtn");
    
    const modal = document.getElementById("reportModal");
    const closeBtn = document.querySelector(".close-btn");
    const incidentForm = document.getElementById("incidentForm");
    
    let map; 
    let miniMap; 
    let reportLat = 0;
    let reportLng = 0;

    // --- 1. 角色视图初始化 ---
    function initRoleView() {
        if (currentUserRole === 'security') {
            // Admin: 显示审批列表，隐藏报案按钮
            if(securityPanel) securityPanel.style.display = "block"; 
            if(reportBtn) reportBtn.style.display = "none";      
            document.body.classList.add('role-security');
        } else {
            // Student: 隐藏审批列表，显示报案按钮
            if(securityPanel) securityPanel.style.display = "none";  
            if(reportBtn) reportBtn.style.display = "block";     
            document.body.classList.add('role-student');
        }
    }
    initRoleView();

    // --- 2. 登出逻辑 ---
    if(logoutBtn) {
        logoutBtn.addEventListener('click', (e) => {
            e.preventDefault();
            alert("Logging out...");
            window.location.href = 'login.html';
        });
    }

    // --- 3. 侧边栏 ---
    sidebarBtn.onclick = function() {
        sidebar.classList.toggle("active");
    }

    navLinks.forEach(link => {
        link.addEventListener("click", (e) => {
            e.preventDefault();
            navLinks.forEach(l => l.classList.remove("active"));
            tabContents.forEach(tc => tc.style.display = "none");

            link.classList.add("active");
            const targetId = link.getAttribute("data-target");
            const targetContent = document.getElementById(targetId);
            
            if (targetContent) {
                targetContent.style.display = "block";
                if (targetId === "incidents-section") setTimeout(() => initMainMap(), 200);
                if (targetId === "dashboard-section") setTimeout(() => initMiniMap(), 200);
            }
        });
    });

    // --- 4. 地图与数据加载 ---
    function initMainMap() {
        if (!map) {
            map = L.map('main-map').setView([2.9276, 101.6413], 16);
            L.tileLayer('https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png', {
                attribution: '© OpenStreetMap'
            }).addTo(map);
            
            // 获取案件数据
            fetch('/get_incidents')
            .then(res => res.text())
            .then(data => {
                const rows = data.split(';');
                const activeTbody = document.getElementById('active-tbody');
                const pendingTbody = document.getElementById('pending-tbody');
                
                rows.forEach(rowStr => {
                    if(!rowStr) return;
                    const [id, type, loc, time, status, lat, lng] = rowStr.split('|');
                    
                    if (status === 'Approved') {
                        // Active Incident (所有人都可见)
                        const tr = document.createElement('tr');
                        
                        // 只有 Admin 有 Resolve 按钮
                        let actionHtml = `<span class="badge info">View Only</span>`;
                        if (currentUserRole === 'security') {
                            actionHtml = `<button class="delete-btn" onclick="resolveIncident('${id}')">Resolve</button>`;
                        }

                        tr.innerHTML = `<td>${type}</td><td>${loc}</td><td>${time}</td><td><span class="badge approved">Active</span></td><td>${actionHtml}</td>`;
                        if(activeTbody) activeTbody.appendChild(tr);
                        
                        // 地图标记
                        if (lat && lng) {
                            L.marker([parseFloat(lat), parseFloat(lng)]).addTo(map)
                                .bindPopup(`<b>${type}</b><br>${loc}`);
                        }
                    } 
                    else if (status === 'Pending' && currentUserRole === 'security') {
                        // Pending Approvals (只有 Admin 可见)
                        const tr = document.createElement('tr');
                        tr.innerHTML = `
                            <td>${type}</td><td>${loc}</td><td>${time}</td>
                            <td>
                                <button class="approve-btn" onclick="postUpdate('${id}', 'approve')">Approve</button>
                                <button class="delete-btn" onclick="postUpdate('${id}', 'reject')">Reject</button>
                            </td>`;
                        if(pendingTbody) pendingTbody.appendChild(tr);
                    }
                });
            });

            if (navigator.geolocation) {
                navigator.geolocation.getCurrentPosition(pos => {
                    const lat = pos.coords.latitude;
                    const lng = pos.coords.longitude;
                    L.circleMarker([lat, lng], { radius: 6, color: 'blue', fillColor: '#3498db', fillOpacity: 0.8 })
                      .addTo(map).bindPopup("You are here");
                });
            }
        } else {
            map.invalidateSize(); 
        }
    }

    function initMiniMap() {
        if (!miniMap) {
            miniMap = L.map('mini-map', { zoomControl: false }).setView([2.9276, 101.6413], 15);
            L.tileLayer('https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png').addTo(miniMap);
            miniMap.dragging.disable();
            miniMap.touchZoom.disable();
            miniMap.doubleClickZoom.disable();
            miniMap.scrollWheelZoom.disable();
        } else {
            miniMap.invalidateSize();
        }
    }

    // --- 5. 报案逻辑 ---
    if(reportBtn) {
        reportBtn.addEventListener('click', () => {
            modal.style.display = "flex";
            const locInput = document.getElementById('inLoc');
            locInput.value = "📍 Locating...";

            if (navigator.geolocation) {
                navigator.geolocation.getCurrentPosition(
                    (position) => {
                        reportLat = position.coords.latitude;
                        reportLng = position.coords.longitude;
                        locInput.value = `📍 GPS (${reportLat.toFixed(4)}, ${reportLng.toFixed(4)})`;
                    },
                    (error) => {
                        locInput.value = "";
                        locInput.placeholder = "Location failed";
                        reportLat = 2.9276; reportLng = 101.6413;
                    }
                );
            } else {
                locInput.placeholder = "Geolocation not supported";
            }
        });
    }

    if(closeBtn) closeBtn.onclick = () => modal.style.display = "none";
    window.onclick = (e) => { if (e.target === modal) modal.style.display = "none"; };

    if(incidentForm) {
        incidentForm.onsubmit = (e) => {
            e.preventDefault();
            const type = document.getElementById('inType').value;
            const loc = document.getElementById('inLoc').value;
            const time = document.getElementById('inTime').value;

            fetch('/report_incident', {
                method: 'POST',
                body: `type=${type}&location=${loc}&time=${time}&lat=${reportLat}&lng=${reportLng}`
            }).then(res => {
                if(res.ok) {
                     document.getElementById('successOverlay').style.display = "flex";
                     setTimeout(() => {
                        document.getElementById('successOverlay').style.display = "none";
                        modal.style.display = "none";
                        incidentForm.reset();
                        alert("Report submitted! Sent to Admin for approval.");
                     }, 1500);
                } else {
                    alert("Failed to submit.");
                }
            });
        };
    }

    window.switchToIncidents = function() {
        document.querySelector('[data-target="incidents-section"]').click();
    };

    // --- 6. 审批与 Resolve 逻辑 ---
    window.postUpdate = function(id, action) {
        fetch('/update_incident', {
            method: 'POST',
            body: `id=${id}&action=${action}`
        }).then(res => {
            if(res.ok) {
                alert("Incident " + action + "d!");
                location.reload(); 
            }
        });
    };
    
    window.resolveIncident = function(id) {
        if(!confirm("Resolve this incident?")) return;
        postUpdate(id, 'reject'); // 逻辑上 resolve = 移除出列表 (Rejected status)
    };
    
    window.getMapInstance = () => map;
});