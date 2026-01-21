document.addEventListener('DOMContentLoaded', () => {
    
    // =========================================================
    // 🔐 AUTH & GRANULAR PERMISSIONS
    // =========================================================
    
    // 1. Get the Raw Role from C++ (e.g., "Student", "Warden", "Security", "Admin")
    const serverRole = window.SERVER_INJECTED_ROLE || 'student'; 
    const roleLower = serverRole.toLowerCase();

    console.log(`Logged in as: ${serverRole}`);

    // 2. Define Capabilities based on Role
    // This fixes the logic flattening where everyone became 'security'
    const permissions = {
        // Can approve/reject incidents (Admin + Security)
        canManageIncidents: roleLower.includes('admin') || roleLower.includes('security'),
        
        // Can start evacuations and mark students safe (Admin + Warden)
        canManageEvacuation: roleLower.includes('admin') || roleLower.includes('warden'),
        
        // Is a regular student (no staff panels)
        isStudent: !roleLower.includes('admin') && !roleLower.includes('security') && !roleLower.includes('warden')
    };

    // =========================================================
    // 🚀 UI ELEMENTS
    // =========================================================

    const sidebar = document.querySelector(".sidebar");
    const sidebarBtn = document.querySelector(".sidebarBtn");
    const navLinks = document.querySelectorAll(".nav-link");
    const tabContents = document.querySelectorAll(".tab-content");
    
    // Panels & Buttons
    const securityPanel = document.getElementById("security-panel");
    const wardenPanel = document.getElementById("warden-panel");
    const reportBox = document.getElementById("report-box");
    
    const reportBtn = document.getElementById("openModalBtn");
    const dashReportBtn = document.getElementById("dashReportBtn");
    const iAmSafeBtn = document.getElementById("iAmSafeBtn");
    const startEvacBtn = document.getElementById("startEvacBtn");
    const genReportBtn = document.getElementById("genReportBtn");
    const evacBlock = document.getElementById("evacBlock");
    const logoutBtn = document.getElementById("logoutBtn");

    // Modal
    const modal = document.getElementById("reportModal");
    const closeBtn = document.querySelector(".close-btn");
    const incidentForm = document.getElementById("incidentForm");

    // Tables
    const activeTbody = document.getElementById('active-tbody');
    const pendingTbody = document.getElementById('pending-tbody');
    const evacuationTbody = document.getElementById("evacuation-tbody");
    
    // Maps
    let map; 
    let miniMap; 
    let markers = [];
    let reportLat = 0;
    let reportLng = 0;

    // --- 1. Role View Initialization ---
    function initRoleView() {
        // Toggle Incident Management Panel (Security/Admin)
        if (securityPanel) {
            securityPanel.style.display = permissions.canManageIncidents ? "block" : "none";
        }

        // Toggle Evacuation Management Panel (Warden/Admin)
        if (wardenPanel) {
            wardenPanel.style.display = permissions.canManageEvacuation ? "block" : "none";
        }

        // Student Specific UI
        if (permissions.isStudent) {
            if(reportBtn) reportBtn.style.display = "block";
            if(reportBox) reportBox.style.display = "flex";
            if(iAmSafeBtn) iAmSafeBtn.style.display = "block";
            document.body.classList.add('role-student');
        } else {
            // Staff/Warden View
            if(reportBtn) reportBtn.style.display = "none";
            if(reportBox) reportBox.style.display = "none";
            if(iAmSafeBtn) iAmSafeBtn.style.display = "none";
            document.body.classList.add('role-staff');
        }
    }
    initRoleView();

    // --- 2. Navigation & Sidebar ---
    if(sidebarBtn) {
        sidebarBtn.onclick = function() {
            sidebar.classList.toggle("active");
        }
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
                
                // Lazy Load Components
                if (targetId === "incidents-section") setTimeout(() => initMainMap(), 200);
                if (targetId === "dashboard-section") setTimeout(() => initMiniMap(), 200);
                if (targetId === "evacuation-section") fetchEvacuationData();
            }
        });
    });

    if(logoutBtn) {
        logoutBtn.addEventListener('click', (e) => {
            e.preventDefault();
            window.location.href = 'login.html';
        });
    }

    // --- 3. Map & Incident Data ---
    function initMainMap() {
        if (!map) {
            map = L.map('main-map').setView([2.9276, 101.6413], 16);
            L.tileLayer('https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png', {
                attribution: '© OpenStreetMap'
            }).addTo(map);
            
            if (navigator.geolocation) {
                navigator.geolocation.getCurrentPosition(pos => {
                    const lat = pos.coords.latitude;
                    const lng = pos.coords.longitude;
                    L.circleMarker([lat, lng], { radius: 6, color: 'blue', fillColor: '#3498db', fillOpacity: 0.8 })
                      .addTo(map).bindPopup("You are here");
                });
            }
            fetchIncidents();
        } else {
            map.invalidateSize(); 
            fetchIncidents(); 
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

    function fetchIncidents() {
        fetch('/get_incidents')
        .then(res => res.text())
        .then(data => {
            if(activeTbody) activeTbody.innerHTML = "";
            if(pendingTbody) pendingTbody.innerHTML = "";
            
            markers.forEach(m => map.removeLayer(m));
            markers = [];

            const rows = data.split(';');
            rows.forEach(rowStr => {
                if(!rowStr) return;
                const [id, type, loc, time, status, lat, lng] = rowStr.split('|');
                
                if (status === 'Approved') {
                    const tr = document.createElement('tr');
                    let actionHtml = `<span class="badge info">View Only</span>`;
                    
                    if (permissions.canManageIncidents) {
                        actionHtml = `<button class="delete-btn" onclick="resolveIncident('${id}')">Resolve</button>`;
                    }

                    tr.innerHTML = `<td>${type}</td><td>${loc}</td><td>${time}</td><td><span class="badge approved">Active</span></td><td>${actionHtml}</td>`;
                    if(activeTbody) activeTbody.appendChild(tr);
                    
                    if (map && lat && lng) {
                        const m = L.marker([parseFloat(lat), parseFloat(lng)]).addTo(map)
                            .bindPopup(`<b>${type}</b><br>${loc}`);
                        markers.push(m);
                    }
                } 
                else if (status === 'Pending' && permissions.canManageIncidents) {
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
    }

    // --- 4. Reporting Logic ---
    const showReportModal = () => {
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
        }
    };

    if(reportBtn) reportBtn.addEventListener('click', showReportModal);
    if(dashReportBtn) dashReportBtn.addEventListener('click', showReportModal);
    if(closeBtn) closeBtn.onclick = () => modal.style.display = "none";

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
                     modal.style.display = "none";
                     incidentForm.reset();
                     alert("Report submitted! Sent to Admin for approval.");
                }
            });
        };
    }

    // --- 5. Evacuation Logic (Fixed for Hostel/Automark) ---
    function fetchEvacuationData() {
        fetch('/get_evacuation').then(res => res.text()).then(data => {
            if(!evacuationTbody) return;
            evacuationTbody.innerHTML = "";
            let missing = 0, safe = 0;
            let amIMissing = false;
            
            const welcomeMsg = document.querySelector('.admin_name') ? document.querySelector('.admin_name').innerText : "";
            const myName = welcomeMsg.replace('Welcome, ', '');

            const rows = data.split(';');
            rows.forEach(row => {
                if(!row.trim()) return;
                const [id, name, block, status, time] = row.split('|');
                
                if(status === 'Safe') safe++; else missing++;
                if(name === myName && status === 'Missing') amIMissing = true;

                let btn = "-";
                if (permissions.canManageEvacuation && status === 'Missing') {
                    btn = `<button class="approve-btn" onclick="markSafe('${id}')">Mark Safe</button>`;
                }
                
                let badge = status === 'Safe' 
                    ? '<span class="badge approved">Safe</span>' 
                    : '<span class="badge warning">Missing</span>';
                
                const tr = document.createElement('tr');
                tr.innerHTML = `<td>${name}</td><td>${block}</td><td>${badge}</td><td>${time}</td><td>${btn}</td>`;
                evacuationTbody.appendChild(tr);
            });

            if(document.getElementById('count-missing')) document.getElementById('count-missing').innerText = missing;
            if(document.getElementById('count-safe')) document.getElementById('count-safe').innerText = safe;

            // Show "I am Safe" only if student is marked 'Missing' by the Warden
            if(permissions.isStudent && iAmSafeBtn) {
                iAmSafeBtn.style.display = amIMissing ? "block" : "none";
            }
        });
    }

    // Fixed: Ensure "ALL" is sent if no block is selected to trigger automark for all hostel students
    if(startEvacBtn) startEvacBtn.onclick = () => {
        if (!permissions.canManageEvacuation) {
             alert("Unauthorized"); return;
        }

        const blkInput = document.getElementById("evacBlock");
        const blk = (blkInput && blkInput.value) ? blkInput.value : "ALL"; 

        if(confirm(`🚨 EMERGENCY: Start Evacuation for ${blk === "ALL" ? "ALL HOSTELS" : "Block " + blk}? \n\nThis will mark all hostel residents as UNSAFE.`)) {
            fetch('/start_evacuation', {
                method: 'POST',
                body: `block=${blk}`
            })
            .then(res => {
                if(res.ok) {
                    alert("⚠️ Evacuation Started! All hostel students marked 'Missing'. Students must self-verify safety."); 
                    fetchEvacuationData();
                }
            });
        }
    };

    if(genReportBtn) genReportBtn.onclick = () => {
        if (!permissions.canManageEvacuation) return;
        fetch('/generate_report', {method:'POST'}).then(()=>alert("Report Generated! Check Server."));
    };

    if(iAmSafeBtn) iAmSafeBtn.onclick = () => { 
        if(confirm("Confirm you are safe?")) {
            fetch('/self_safe', {method:'POST'}).then(()=>fetchEvacuationData()); 
        }
    };

    // --- 6. Global Actions ---
    window.postUpdate = function(id, action) {
        if (!permissions.canManageIncidents) return;
        fetch('/update_incident', {
            method: 'POST',
            body: `id=${id}&action=${action}`
        }).then(res => {
            if(res.ok) {
                alert("Incident " + action + "d!");
                fetchIncidents();
            }
        });
    };
    
    window.resolveIncident = function(id) {
        if (!permissions.canManageIncidents) return;
        if(!confirm("Resolve this incident?")) return;
        postUpdate(id, 'reject');
    };

    window.markSafe = function(id) {
        if (!permissions.canManageEvacuation) return;
        fetch('/mark_safe', {method:'POST', body:`id=${id}`})
        .then(()=>fetchEvacuationData());
    }

    setTimeout(initMiniMap, 500);
});