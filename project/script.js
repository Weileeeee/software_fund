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

    function checkEmergencyStatus() {
        fetch('/get_resident_list') // Or your specific status endpoint
            .then(response => response.json())
            .then(data => {
                // Find current user in the list and check if status is "Missing" or "Evacuation"
                const me = data.find(s => s.name === window.SERVER_INJECTED_USER);
                if (me && (me.status === "Missing" || me.status === "Evacuation")) {
                    showEmergencyOverlay();
                }
            });
    }

    function showEmergencyOverlay() {
        if (document.getElementById('emergency-popup')) return; // Don't show twice

        const overlay = document.createElement('div');
        overlay.id = 'emergency-popup';
        overlay.innerHTML = `
        <div style="position:fixed; top:0; left:0; width:100%; height:100%; background:rgba(255,0,0,0.9); 
                    z-index:9999; color:white; display:flex; flex-direction:column; align-items:center; justify-content:center; text-align:center;">
            <h1 style="font-size: 4rem;">🚨 EMERGENCY EVACUATION 🚨</h1>
            <p style="font-size: 2rem;">Please proceed to the assembly point immediately!</p>
            <button onclick="markSafe()" style="padding:20px 40px; font-size:1.5rem; cursor:pointer; margin-top:20px;">I AM SAFE</button>
        </div>
    `;
        document.body.appendChild(overlay);
    }

    window.markSafe = function () {
        fetch('/mark_safe', { method: 'POST' })
            .then(() => {
                document.getElementById('emergency-popup').remove();
                alert("Status updated to Safe.");
            });
    };

    // Start checking every 5 seconds if the user is a student
    if (permissions.isStudent) {
        setInterval(checkEmergencyStatus, 5000);
    }

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
            if (reportBox) reportBox.style.display = "flex";
            if (iAmSafeBtn) iAmSafeBtn.style.display = "block";
            document.body.classList.add('role-student');
        } else {
            // Staff/Warden View
            if (reportBox) reportBox.style.display = "none";
            if (iAmSafeBtn) iAmSafeBtn.style.display = "none";
            document.body.classList.add('role-staff');
        }

        // ENABLE REPORTING FOR ALL ROLES (Admin, Warden, Security, Student)
        if (reportBtn) reportBtn.style.display = "block";

        // Show Warden Specific Dashboard Panel
        const wardenDashPanel = document.getElementById("warden-dash-panel");
        if (wardenDashPanel) {
            wardenDashPanel.style.display = permissions.canManageEvacuation ? "flex" : "none";
        }

        // VISIBILITY: Allow Students to see Evacuation to mark themselves safe
        // Residence is typically Warden only, but Evacuation needs to be public for self-safety check.
        const evLink = document.querySelector('.nav-link[data-target="evacuation-section"]');
        const resLink = document.querySelector('.nav-link[data-target="residence-section"]');

        // Evacuation: Visible to Wardens AND Students (everyone)
        if (evLink && evLink.parentElement) {
            evLink.parentElement.style.display = "block";
        }

        // Residence: Warden Only (Keep hidden for students if desired, or ask user. User said "students also can view", possibly implying both?)
        // User request: "undo the change about only warden can view evacuation part , make students also can view and mark them themselves safe"
        // It implies Evacuation is the key one. I will leave Residence as Warden-only unless specified, 
        // but the prompt said "undo the change about only warden can view evacuation part".
        // Use logic: Residence -> Warden Only. Evacuation -> All.
        if (resLink && resLink.parentElement) {
            resLink.parentElement.style.display = permissions.canManageEvacuation ? "block" : "none";
        }
    }
    initRoleView();

    // --- 2. Navigation & Sidebar ---
    if (sidebarBtn) {
        sidebarBtn.onclick = function () {
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
                if (targetId === "residence-section") fetchResidenceData();
            }
        });
    });

    if (logoutBtn) {
        logoutBtn.addEventListener('click', (e) => {
            e.preventDefault();
            window.location.href = '/login.html';
        });
    }

    // --- 3. Modal Handlers ---
    if (reportBtn) reportBtn.onclick = () => modal.style.display = "flex";
    if (dashReportBtn) dashReportBtn.onclick = () => modal.style.display = "flex";
    if (closeBtn) closeBtn.onclick = () => modal.style.display = "none";
    window.onclick = (e) => { if (e.target === modal) modal.style.display = "none"; };

    // --- 4. Incident Form Submission ---
    if (incidentForm) {
        incidentForm.addEventListener("submit", function (e) {
            e.preventDefault();

            const formData = new FormData(this);
            const lat = reportLat || 0;
            const lng = reportLng || 0;
            const params = `type=${formData.get('type')}&location=${formData.get('location')}&time=${formData.get('time')}&lat=${lat}&lng=${lng}&description=${formData.get('description')}&severity=${formData.get('severity')}&evidence=${formData.get('evidence')}`;

            fetch('/report_incident', { method: 'POST', body: params })
                .then(res => res.text())
                .then(data => {
                    if (data && data !== "Fail") {
                        alert("✅ Incident Reported Successfully! ID: " + data);
                        incidentForm.reset();
                        modal.style.display = "none";
                        fetchIncidents();
                    } else {
                        alert("❌ Failed to report incident");
                    }
                });
        });
    }

    // --- 5. Mini Map (Dashboard) ---
    function initMiniMap() {
        const miniMapEl = document.getElementById("mini-map");
        if (!miniMapEl || miniMap) return;
        miniMap = L.map("mini-map").setView([1.4927, 103.6422], 14);
        L.tileLayer('https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png', { maxZoom: 18 }).addTo(miniMap);
        fetch('/get_incidents')
            .then(res => res.text())
            .then(data => {
                const rows = data.split(';');
                rows.forEach(r => {
                    if (!r) return;
                    const [id, type, loc, time, status] = r.split('|');
                    // For demo purposes, we'll place pins near UTHM
                    const lat = 1.4927 + (Math.random() - 0.5) * 0.05;
                    const lng = 103.6422 + (Math.random() - 0.5) * 0.05;
                    L.marker([lat, lng]).addTo(miniMap).bindPopup(`<b>${type}</b><br>${loc}<br>${status}`);
                });
            });
    }

    // --- 6. Main Map (Incidents) ---
    function initMainMap() {
        const mainMapEl = document.getElementById("main-map");
        if (!mainMapEl || map) return;
        map = L.map("main-map").setView([1.4927, 103.6422], 15);
        L.tileLayer('https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png', { maxZoom: 18 }).addTo(map);

        map.on('click', function (e) {
            markers.forEach(m => map.removeLayer(m));
            markers = [];
            const marker = L.marker([e.latlng.lat, e.latlng.lng]).addTo(map);
            markers.push(marker);
            reportLat = e.latlng.lat;
            reportLng = e.latlng.lng;
            document.getElementById("reportLocation").value = `${e.latlng.lat.toFixed(5)}, ${e.latlng.lng.toFixed(5)}`;
        });

        fetchIncidents();
    }

    function fetchIncidents() {
        fetch('/get_incidents')
            .then(res => res.text())
            .then(data => {
                if (pendingTbody) pendingTbody.innerHTML = "";
                if (activeTbody) activeTbody.innerHTML = "";

                const rows = data.split(';');
                rows.forEach(r => {
                    if (!r) return;
                    const [id, type, loc, time, status, reporter] = r.split('|');
                    const tr = document.createElement('tr');
                    tr.innerHTML = `
                        <td>${type}</td>
                        <td>${loc}</td>
                        <td>${time}</td>
                        <td>${reporter}</td>
                        <td><span class="badge ${status === 'Pending' ? 'warning' : (status === 'Approved' ? 'success' : 'danger')}">${status}</span></td>
                    `;

                    if (permissions.canManageIncidents && status === 'Pending') {
                        tr.innerHTML += `<td>
                            <button class="btn-action btn-safe" onclick="approveIncident('${id}')">✔ Approve</button>
                            <button class="btn-action btn-danger" onclick="resolveIncident('${id}')">✖ Reject</button>
                        </td>`;
                    }

                    if (status === 'Pending' && pendingTbody) pendingTbody.appendChild(tr);
                    else if (activeTbody) activeTbody.appendChild(tr);
                });
            });
    }

    // --- ENHANCED: EVACUATION WITH NOTIFICATION FEEDBACK ---
    function fetchEvacuationData() {
        if (!evacuationTbody) return;
        fetch('/get_evacuation')
            .then(res => res.text())
            .then(data => {
                evacuationTbody.innerHTML = "";
                const rows = data.split(';');

                let safeCount = 0;
                let missingCount = 0;

                rows.forEach(r => {
                    if (!r) return;
                    const [id, name, block, status] = r.split('|');

                    if (status === 'Safe') safeCount++;
                    else missingCount++;

                    const tr = document.createElement('tr');
                    tr.innerHTML = `
                        <td>${name}</td>
                        <td>${block}</td>
                        <td><span class="badge ${status === 'Safe' ? 'success' : ((status === 'Not Safe' || status === 'Missing') ? 'danger' : 'warning')}">${status}</span></td>
                    `;

                    // Warden/Admin can mark students safe
                    if (permissions.canManageEvacuation) {
                        tr.innerHTML += `<td><button class="btn-action btn-safe" onclick="markSafe('${id}')">Mark Safe</button></td>`;
                    }

                    evacuationTbody.appendChild(tr);
                });

                // Update Overview Counters
                const missingEl = document.getElementById('count-missing');
                const safeEl = document.getElementById('count-safe');
                if (missingEl) missingEl.innerText = missingCount;
                if (safeEl) safeEl.innerText = safeCount;
            });
    }

    // ENHANCED: Start Evacuation with Loading and Notification Feedback
    if (startEvacBtn) {
        startEvacBtn.addEventListener('click', () => {
            const block = evacBlock.value;
            if (!block) {
                alert("⚠️ Please select a block");
                return;
            }

            // Confirmation dialog with clear messaging
            if (!confirm(`🚨 START EVACUATION FOR ${block}?\n\n` +
                `This will:\n` +
                `• Set all students in ${block} to "Not Safe" status\n` +
                `• Send WhatsApp notifications to all students\n` +
                `• Alert students to evacuate immediately\n\n` +
                `Are you sure you want to proceed?`)) {
                return;
            }

            // Disable button and show loading state
            startEvacBtn.disabled = true;
            startEvacBtn.textContent = "📤 Sending Notifications...";
            startEvacBtn.style.opacity = "0.6";

            // Show progress overlay
            showNotificationProgress();

            fetch('/start_evacuation', {
                method: 'POST',
                headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
                body: `block=${encodeURIComponent(block)}`
            })
                .then(res => res.text())
                .then(data => {
                    // Re-enable button
                    startEvacBtn.disabled = false;
                    startEvacBtn.textContent = "🚨 Start Evacuation";
                    startEvacBtn.style.opacity = "1";

                    // Hide progress overlay
                    hideNotificationProgress();

                    if (data === 'Started') {
                        // Success notification with details
                        showSuccessNotification(
                            `✅ EVACUATION STARTED FOR ${block}!`,
                            `• WhatsApp notifications have been sent to all students\n` +
                            `• Students have been alerted to evacuate immediately\n` +
                            `• All safety statuses have been reset to "Not Safe"\n` +
                            `• Students can mark themselves safe using the app\n\n` +
                            `Check the server console for detailed delivery status.`
                        );

                        // Refresh the evacuation list
                        fetchEvacuationData();

                        // Play alert sound if available
                        playAlertSound();
                    } else {
                        alert('❌ Failed to start evacuation. Please check the server console for errors.');
                    }
                })
                .catch(err => {
                    // Re-enable button on error
                    startEvacBtn.disabled = false;
                    startEvacBtn.textContent = "🚨 Start Evacuation";
                    startEvacBtn.style.opacity = "1";
                    hideNotificationProgress();

                    alert('❌ Error starting evacuation: ' + err);
                    console.error('Evacuation error:', err);
                });
        });
    }

    // Helper function to show notification progress overlay
    function showNotificationProgress() {
        const overlay = document.createElement('div');
        overlay.id = 'notification-progress-overlay';
        overlay.innerHTML = `
            <div style="position: fixed; top: 0; left: 0; width: 100%; height: 100%; 
                        background: rgba(0, 0, 0, 0.8); z-index: 10000; 
                        display: flex; align-items: center; justify-content: center;">
                <div style="background: white; padding: 40px; border-radius: 10px; text-align: center; 
                            max-width: 400px; box-shadow: 0 4px 20px rgba(0,0,0,0.3);">
                    <div style="font-size: 60px; margin-bottom: 20px;">📱</div>
                    <h2 style="margin: 0 0 15px 0; color: #333;">Sending Notifications</h2>
                    <p style="color: #666; margin: 0 0 20px 0;">
                        WhatsApp messages are being sent to all students in the selected block...
                    </p>
                    <div style="width: 100%; height: 4px; background: #eee; border-radius: 2px; overflow: hidden;">
                        <div style="width: 100%; height: 100%; background: linear-gradient(90deg, #4CAF50, #8BC34A); 
                                    animation: progress 2s ease-in-out infinite;"></div>
                    </div>
                    <p style="color: #999; margin: 20px 0 0 0; font-size: 14px;">
                        This may take a few moments depending on the number of students
                    </p>
                </div>
            </div>
            <style>
                @keyframes progress {
                    0% { transform: translateX(-100%); }
                    100% { transform: translateX(100%); }
                }
            </style>
        `;
        document.body.appendChild(overlay);
    }

    // Helper function to hide notification progress overlay
    function hideNotificationProgress() {
        const overlay = document.getElementById('notification-progress-overlay');
        if (overlay) {
            overlay.remove();
        }
    }

    // Helper function to show success notification with auto-close
    function showSuccessNotification(title, message) {
        const notification = document.createElement('div');
        notification.innerHTML = `
            <div style="position: fixed; top: 50%; left: 50%; transform: translate(-50%, -50%); 
                        background: white; padding: 30px; border-radius: 10px; 
                        box-shadow: 0 4px 20px rgba(0,0,0,0.3); z-index: 10001; 
                        max-width: 500px; text-align: center;">
                <div style="font-size: 60px; margin-bottom: 20px;">✅</div>
                <h2 style="margin: 0 0 15px 0; color: #4CAF50;">${title}</h2>
                <p style="color: #666; white-space: pre-line; margin: 0 0 20px 0; text-align: left;">
                    ${message}
                </p>
                <button onclick="this.parentElement.remove()" 
                        style="background: #4CAF50; color: white; border: none; 
                               padding: 12px 30px; border-radius: 5px; cursor: pointer; 
                               font-size: 16px; font-weight: bold;">
                    OK
                </button>
            </div>
        `;
        document.body.appendChild(notification);

        // Auto-close after 10 seconds
        setTimeout(() => {
            if (notification.parentElement) {
                notification.remove();
            }
        }, 10000);
    }

    // Helper function to play alert sound
    function playAlertSound() {
        try {
            // Create a simple beep sound using Web Audio API
            const audioContext = new (window.AudioContext || window.webkitAudioContext)();
            const oscillator = audioContext.createOscillator();
            const gainNode = audioContext.createGain();

            oscillator.connect(gainNode);
            gainNode.connect(audioContext.destination);

            oscillator.frequency.value = 800;
            oscillator.type = 'sine';

            gainNode.gain.setValueAtTime(0.3, audioContext.currentTime);
            gainNode.gain.exponentialRampToValueAtTime(0.01, audioContext.currentTime + 0.5);

            oscillator.start(audioContext.currentTime);
            oscillator.stop(audioContext.currentTime + 0.5);
        } catch (e) {
            console.log('Could not play alert sound:', e);
        }
    }

    // Generate Report Button
    if (genReportBtn) {
        genReportBtn.addEventListener('click', () => {
            fetch('/generate_report', { method: 'POST' })
                .then(() => alert("✅ Report Generated Successfully!\n\nThe unsafe students report has been saved to:\nunsafe_students_report.txt"));
        });
    }

    // Student Self-Report Safe Button
    if (iAmSafeBtn) {
        iAmSafeBtn.addEventListener('click', () => {
            if (!confirm("Mark yourself as SAFE?")) return;

            fetch('/self_safe', { method: 'POST' })
                .then(res => res.text())
                .then(data => {
                    if (data === 'Updated') {
                        alert("✅ Your status has been updated to SAFE!\n\nThank you for confirming your safety.");
                        fetchEvacuationData();
                    } else {
                        alert("❌ Failed to update status. Please try again.");
                    }
                });
        });
    }

    // Incident Management Functions
    window.approveIncident = function (id) {
        if (!permissions.canManageIncidents) return;
        if (!confirm("Approve this incident?")) return;

        postUpdate(id, 'approve');
    };

    function postUpdate(id, action) {
        fetch('/update_incident', {
            method: 'POST',
            body: `id=${id}&action=${action}`
        }).then(res => res.text()).then(data => {
            if (data === 'Updated') {
                alert("✅ Incident status updated");
                fetchIncidents();
            }
        });
    };

    window.resolveIncident = function (id) {
        if (!permissions.canManageIncidents) return;
        if (!confirm("Resolve this incident?")) return;
        postUpdate(id, 'reject');
    };

    window.markSafe = function (id) {
        if (!permissions.canManageEvacuation) return;
        fetch('/mark_safe', { method: 'POST', body: `id=${id}` })
            .then(() => fetchEvacuationData());
    }

    // --- 7. RESIDENCE MONITORING ---
    const resTbody = document.getElementById('residence-tbody');
    const resSearch = document.getElementById('resSearch');
    const statusModal = document.getElementById('statusModal');



    const statusForm = document.getElementById('statusForm');

    function fetchResidenceData() {
        if (!resTbody) return;
        fetch('/get_residents')
            .then(res => res.text())
            .then(data => {
                resTbody.innerHTML = "";
                const rows = data.split(';');
                rows.forEach(row => {
                    if (!row) return;
                    const [id, name, room, status, email] = row.split('|');
                    const tr = document.createElement('tr');
                    tr.innerHTML = `
                    <td>${name}</td>
                    <td>${room}</td>
                    <td><span class="badge info">${status}</span></td>
                    <td><a href="mailto:${email}">${email}</a></td>
                    <td><button class="btn-action btn-safe" onclick="openStatusModal('${id}', '${name}')">Update</button></td>
                `;
                    resTbody.appendChild(tr);
                });
            });
    }

    if (resSearch) {
        resSearch.addEventListener('keyup', () => {
            const term = resSearch.value.toLowerCase();
            const rows = resTbody.getElementsByTagName('tr');
            Array.from(rows).forEach(row => {
                const name = row.cells[0].innerText.toLowerCase();
                const room = row.cells[1].innerText.toLowerCase();
                if (name.includes(term) || room.includes(term)) {
                    row.style.display = "";
                } else {
                    row.style.display = "none";
                }
            });
        });
    }

    window.openStatusModal = function (id, name) {
        document.getElementById('statusUserId').value = id;
        document.getElementById('statusUserName').innerText = "Update: " + name;
        statusModal.style.display = "flex";
    };

    window.closeStatusModal = function () {
        statusModal.style.display = "none";
    };

    // --- 8. LECTURER PORTAL LOGIC ---
    // (Only runs if elements exist, i.e., on lecturer.html)

    function fetchBroadcasts() {
        fetch('/get_broadcasts')
            .then(res => res.text())
            .then(data => {
                const container = document.querySelector('.recent-alerts form'); // Just to find the section? No, let's find a list container.
                // In lecturer.html, there might be a list or just the form. Feature request imply viewing them too?
                // The source code showed a 'dashboard-broadcasts' div, let's assume it exists or create one below form
                let output = document.getElementById('broadcast-list');
                if (!output) {
                    const section = document.getElementById('communication-section');
                    if (section) {
                        output = document.createElement('div');
                        output.id = 'broadcast-list';
                        output.style.marginTop = "20px";
                        section.querySelector('.recent-alerts').appendChild(output);
                    }
                }

                if (output) {
                    output.innerHTML = "<h3>Sent Broadcasts</h3>";
                    const rows = data.split(';');
                    rows.forEach(r => {
                        if (!r) return;
                        const [id, to, msg, time, by] = r.split('|');
                        const div = document.createElement('div');
                        div.className = "box";
                        div.style.marginBottom = "10px";
                        div.innerHTML = `
                            <div class="right-side" style="width:100%">
                                <div class="box-topic">To: ${to} <small style="float:right">${time}</small></div>
                                <p>${msg}</p>
                                <button onclick="deleteBroadcast('${id}')" style="background:#e74c3c; color:white; border:none; padding:5px; border-radius:3px; cursor:pointer; margin-top:5px;">Delete</button>
                            </div>
                        `;
                        output.appendChild(div);
                    });
                }
            });
    }

    window.deleteBroadcast = function (id) {
        if (confirm("Delete this broadcast?")) {
            fetch('/delete_broadcast', { method: 'POST', body: `id=${id}` })
                .then(() => fetchBroadcasts());
        }
    };

    function fetchClassStudents() {
        const tbody = document.querySelector('#student-status-section tbody');
        if (!tbody) return;
        fetch('/get_students').then(res => res.text()).then(data => {
            tbody.innerHTML = "";
            const rows = data.split(';');
            rows.forEach(r => {
                if (!r) return;
                const [code, name, loc, status] = r.split('|');
                const tr = document.createElement('tr');
                tr.innerHTML = `
                    <td>${code}</td>
                    <td>${name}</td>
                    <td>${loc}</td>
                    <td><span class="badge ${status === 'Safe' ? 'success' : (status === 'Missing' ? 'warning' : 'info')}">${status}</span></td>
                    <td>
                        <select onchange="updateStudentStatus('${code}', this.value)">
                            <option value="">Update Status...</option>
                            <option value="Safe">Mark Safe</option>
                            <option value="Missing">Mark Missing</option>
                            <option value="Injured">Requires Aid</option>
                        </select>
                    </td>
                `;
                tbody.appendChild(tr);
            });
        });
    }

    window.updateStudentStatus = function (code, status) {
        if (!status) return;
        fetch('/update_student_status', {
            method: 'POST',
            body: `student_id=${code}&status=${status}`
        }).then(() => {
            alert("Status Updated");
            fetchClassStudents();
        });
    };

    const broadcastForm = document.getElementById('broadcastForm');
    if (broadcastForm) {
        broadcastForm.onsubmit = function (e) {
            e.preventDefault();
            const to = this.querySelector('select').value;
            const msg = this.querySelector('textarea').value;
            fetch('/broadcast', {
                method: 'POST',
                body: `to=${to}&message=${msg}`
            }).then(() => {
                alert("Broadcast Sent!");
                this.reset();
                fetchBroadcasts();
            });
        };
        // Initial load
        fetchBroadcasts();
        fetchClassStudents();
    }

    // Check if on lecturer page
    if (document.querySelector('.logo_name') && document.querySelector('.logo_name').innerText.includes("Lecturer")) {
        fetchClassStudents();
        fetchBroadcasts();
    }

    setTimeout(initMiniMap, 500);
});