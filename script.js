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
        // FIXED: Use /get_evacuation instead of non-existent /get_resident_list
        fetch('/get_evacuation')
            .then(response => response.text()) // Expect text/plain from C++ server
            .then(data => {
                // Parse the pipe-delimited format: id|name|block|status
                const rows = data.split(';');
                const me = rows.find(r => r.includes('|' + window.SERVER_INJECTED_USER + '|')); // flexible match

                if (me) {
                    const parts = me.split('|');
                    const status = parts[3]; // 0=id, 1=name, 2=block, 3=status

                    if (status === "Missing" || status === "Evacuation" || status === "Not Safe") {
                        showEmergencyOverlay();
                    }
                }
            })
            .catch(err => console.log("Status check failed:", err));
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

    // Mark student as safe (Self-reporting or Warden action)
    window.markSafe = function (id = null) {
        if (id) {
            // Warden marking student safe
            console.log("Marking safe for ID:", id);
            // alert("Debug: Attempting to mark safe for ID: " + id); 
            if (!permissions.canManageEvacuation) {
                alert("Error: You do not have permission to manage evacuation.");
                return;
            }
            fetch('/mark_safe', {
                method: 'POST',
                headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
                body: `id=${id}`
            })
                .then(response => {
                    if (!response.ok) throw new Error('Failed to mark safe');
                    return response.text();
                })
                .then(data => {
                    console.log("Server response:", data);
                    alert("Success: Student marked as Safe.");
                    fetchEvacuationData();
                })
                .catch(error => {
                    console.error('Error marking student safe:', error);
                    alert("Error marking safe: " + error.message);
                });
        } else {
            // Student self-reporting safe
            fetch('/self_safe', { method: 'POST' })
                .then(response => {
                    if (!response.ok) throw new Error('Failed to mark self as safe');
                    const popup = document.getElementById('emergency-popup');
                    if (popup) popup.remove();
                    alert("Status updated to Safe.");
                    // Refresh evacuation data to show updated status
                    fetchEvacuationData();
                })
                .catch(error => {
                    console.error('Error marking self safe:', error);
                    alert("Failed to mark safe. Please try again.");
                });
        }
    }

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
    let incidentLayer; // Layer for incidents on main map
    let miniIncidentLayer; // Layer for incidents on mini map
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
                if (targetId === "lecturer-evacuation-section") fetchClassStudents();
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
            // Add defaults for fields not in the basic form
            const desc = formData.get('description') || 'No description provided';
            const sev = formData.get('severity') || 'Medium';
            const evid = formData.get('evidence') || '';

            let timeVal = formData.get('time');
            if (timeVal && timeVal.length <= 5) {
                const now = new Date();
                timeVal = now.toISOString().split('T')[0] + " " + timeVal;
            }

            const params = `type=${formData.get('type')}&location=${formData.get('location')}&time=${timeVal}&lat=${lat}&lng=${lng}&description=${desc}&severity=${sev}&evidence=${evid}`;

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
        miniIncidentLayer = L.layerGroup().addTo(miniMap);

        fetch('/get_incidents')
            .then(res => res.text())
            .then(data => {
                const rows = data.split(';');
                rows.forEach(r => {
                    if (!r) return;
                    const [id, type, loc, time, status, rep, lat, lng] = r.split('|');

                    if (status === 'Approved' && lat && lng && parseFloat(lat) !== 0) {
                        L.marker([parseFloat(lat), parseFloat(lng)])
                            .addTo(miniIncidentLayer)
                            .bindPopup(`<b>${type}</b><br>${loc}<br><span class="badge success">${status}</span>`);
                    }
                });
            });

    }

    // --- 6. Main Map (Incidents) ---
    function initMainMap() {
        // Support both main-map (dashboard) and lecturer-map (lecturer page)
        let mainMapEl = document.getElementById("main-map");
        let mapContainerId = "main-map";

        if (!mainMapEl) {
            mainMapEl = document.getElementById("lecturer-map");
            mapContainerId = "lecturer-map";
        }

        if (!mainMapEl) {
            console.log("Map container not found (neither main-map nor lecturer-map)");
            return;
        }

        // Reset map if already initialized
        if (map && map._container?.id === mapContainerId) {
            console.log("Map already initialized for " + mapContainerId);
            return;
        }

        console.log("Initializing map with container: " + mapContainerId);

        // Create new map instance
        map = L.map(mapContainerId).setView([1.4927, 103.6422], 15);
        L.tileLayer('https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png', { maxZoom: 18 }).addTo(map);

        // --- LIVE TRACKING START ---
        if (navigator.geolocation) {
            const myIcon = L.divIcon({
                className: 'my-location-marker',
                html: '<div style="background-color: #4285F4; width: 15px; height: 15px; border-radius: 50%; border: 3px solid white; box-shadow: 0 0 5px rgba(0,0,0,0.5);"></div>',
                iconSize: [20, 20]
            });

            let myMarker = null;

            navigator.geolocation.watchPosition((pos) => {
                const lat = pos.coords.latitude;
                const lng = pos.coords.longitude;

                if (!myMarker) {
                    myMarker = L.marker([lat, lng], { icon: myIcon }).addTo(map).bindPopup("You are here");
                    map.setView([lat, lng], 16); // Center on first find
                } else {
                    myMarker.setLatLng([lat, lng]);
                }

                // AUTO-FILL LOCATION IN FORM
                const locInput = document.getElementById("reportLocation");
                if (locInput && !locInput.value) { // Only if empty
                    locInput.value = `GPS (${lat.toFixed(4)}, ${lng.toFixed(4)})`;
                }

                // Keep global vars updated for form submission
                reportLat = lat;
                reportLng = lng;

                // Update report button logic to use 'My Location'?
                // Optional: Add a button to center map on self
            }, (err) => {
                console.log("Location access denied or error: " + err.message);
            }, {
                enableHighAccuracy: true,
                maximumAge: 10000,
                timeout: 20000
            });
        }
        // --- LIVE TRACKING END ---

        // Initialize incident layer
        incidentLayer = L.layerGroup().addTo(map);

        fetchIncidents();
    }

    function fetchIncidents() {
        // Fetch incidents for the table table
        fetch('/get_incidents')
            .then(res => res.text())
            .then(data => {
                if (pendingTbody) pendingTbody.innerHTML = "";
                if (activeTbody) activeTbody.innerHTML = "";

                if (incidentLayer) incidentLayer.clearLayers();

                const rows = data.split(';');
                rows.forEach(r => {
                    if (!r) return;

                    const [id, type, loc, time, status, reporter, lat, lng, detail] = r.split('|');

                    // Add to map if approved
                    if (status === 'Approved' && incidentLayer && lat && lng && parseFloat(lat) !== 0) {
                        L.marker([parseFloat(lat), parseFloat(lng)])
                            .addTo(incidentLayer)
                            .bindPopup(`<b>${type}</b><br>${loc}<br><span class="badge success">${status}</span>${detail ? '<br><i>' + detail + '</i>' : ''}`);
                    }

                    const tr = document.createElement('tr');
                    tr.innerHTML = `
                        <td>${type}</td>
                        <td>${loc}${detail ? '<br><small style="color:#666">Note: ' + detail + '</small>' : ''}</td>
                        <td>${time}</td>
                        <td>${reporter}</td>
                        <td><span class="badge ${status === 'Pending' ? 'warning' : (status === 'Approved' ? 'success' : 'danger')}">${status}</span></td>
                    `;

                    if (permissions.canManageIncidents) {
                        if (status === 'Pending') {
                            tr.innerHTML += `<td>
                                <button class="btn-action btn-safe" onclick="approveIncident('${id}')">✔ Approve</button>
                                <button class="btn-action btn-danger" onclick="resolveIncident('${id}')">✖ Reject</button>
                            </td>`;
                        } else if (status === 'Approved') {
                            tr.innerHTML += `<td>
                                <button class="btn-action btn-danger" onclick="resolveIncident('${id}')">✖ Resolve</button>
                            </td>`;
                        }
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
                            `• WhatsApp messages have been sent to all students\n` +
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
    // Warden Incident Modal Trigger
    const wardenReportBtn = document.getElementById("wardenReportBtn");
    const wardenReportModal = document.getElementById("wardenReportModal");
    const closeWardenModal = document.getElementById("closeWardenModal");
    const wardenForm = document.getElementById("wardenIncidentForm");

    if (wardenReportBtn) {
        wardenReportBtn.addEventListener("click", () => {
            if (wardenReportModal) wardenReportModal.style.display = "flex";

            // Populate incident dropdown
            const dropdown = document.getElementById("wIncidentId");
            if (dropdown) {
                // Keep only the first option: "-- Select Existing Incident --"
                while (dropdown.options.length > 1) dropdown.remove(1);

                fetch('/get_incidents')
                    .then(r => r.text())
                    .then(data => {
                        const rows = data.split(';');
                        rows.forEach(row => {
                            if (!row) return;
                            const parts = row.split('|');
                            if (parts.length >= 3) {
                                const opt = document.createElement("option");
                                opt.value = parts[0];
                                opt.textContent = `#${parts[0]}: ${parts[1]} at ${parts[2]}`;
                                dropdown.appendChild(opt);
                            }
                        });
                    });
            }
        });
    }


    if (closeWardenModal) {
        closeWardenModal.addEventListener("click", () => {
            if (wardenReportModal) wardenReportModal.style.display = "none";
        });
    }

    if (wardenForm) {
        wardenForm.addEventListener("submit", (e) => {
            e.preventDefault();
            const incident_id = document.getElementById("wIncidentId").value;
            const details = document.getElementById("wInDetails").value;

            const body = `incident_id=${incident_id}&details=${encodeURIComponent(details)}`;

            fetch('/save_warden_report', {
                method: 'POST',
                headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
                body: body
            })
                .then(res => res.text())
                .then(data => {
                    if (data.includes("Success")) {
                        alert("✅ Warden Report Saved Successfully!");
                        wardenReportModal.style.display = "none";
                        wardenForm.reset();
                        fetchIncidents(); // Refresh list/map
                    } else {
                        alert("❌ Error saving warden report: " + data);
                    }
                })
                .catch(err => {
                    console.error("Fetch Error:", err);
                    alert("Submission failed. Check connection.");
                });
        });
    }

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

    // Lecturer: Start Class Evacuation (Warden-like for lecturer)
    const startClassBtn = document.getElementById('startClassEvacBtn');
    const lecEvacClass = document.getElementById('lecEvacClass');
    const lecGenReportBtn = document.getElementById('lecGenReportBtn');

    if (startClassBtn) {
        startClassBtn.addEventListener('click', () => {
            const className = (lecEvacClass && lecEvacClass.value) ? lecEvacClass.value : 'All';
            if (!confirm(`🚨 START EVACUATION FOR CLASS: ${className}?\n\nThis will mark students' evacuation status and send notifications.`)) return;

            startClassBtn.disabled = true;
            startClassBtn.style.opacity = '0.6';
            startClassBtn.textContent = '📤 Sending Notifications...';
            showNotificationProgress();

            fetch('/start_class_evacuation', {
                method: 'POST',
                headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
                body: `class=${encodeURIComponent(className)}`
            })
                .then(res => {
                    console.log(`Evacuation endpoint response status: ${res.status}`);
                    return res.text();
                })
                .then(data => {
                    console.log(`Evacuation response (raw): "${data}"`);

                    startClassBtn.disabled = false;
                    startClassBtn.style.opacity = '1';
                    startClassBtn.textContent = '🚨 START CLASS EVACUATION';
                    hideNotificationProgress();

                    // Check for success - accept "Started" or HTTP 200
                    if (data && (data.includes('Started') || data.includes('Success'))) {
                        showSuccessNotification(
                            `✅ EVACUATION STARTED FOR ${className}!`,
                            `• WhatsApp messages have been sent to students in ${className}\n` +
                            `• Attendance statuses updated.\n\nCheck the server console for delivery details.`
                        );
                        if (typeof fetchClassStudents === 'function') fetchClassStudents();
                    } else if (data && data.includes('Fail')) {
                        alert('❌ Failed to start class evacuation.\n\nPossible reasons:\n• Students not properly configured in database\n• WhatsApp integration not working\n\nCheck server logs for details.');
                        console.error('Evacuation failed:', data);
                    } else {
                        console.warn(`Unexpected evacuation response: ${data}`);
                        alert('❌ Unexpected server response. Please check server logs.');
                    }
                })
                .catch(err => {
                    startClassBtn.disabled = false;
                    startClassBtn.style.opacity = '1';
                    startClassBtn.textContent = '🚨 START CLASS EVACUATION';
                    hideNotificationProgress();
                    console.error('Class evacuation error:', err);
                    alert('❌ Network error: ' + err.message);
                });
        });
    }

    // Lecturer: Generate report (reuse endpoint)
    if (lecGenReportBtn) {
        lecGenReportBtn.addEventListener('click', () => {
            fetch('/generate_report', { method: 'POST' })
                .then(() => alert("✅ Lecturer report generated: unsafe_students_report.txt"));
        });
    }

    // Student Self-Report Safe Button
    if (iAmSafeBtn) {
        iAmSafeBtn.addEventListener('click', () => {
            if (!confirm("Mark yourself as SAFE?")) return;

            // Call the unified markSafe function without an ID
            window.markSafe();
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

    if (statusForm) {
        statusForm.addEventListener('submit', (e) => {
            e.preventDefault();
            const userId = document.getElementById('statusUserId').value;
            const status = document.getElementById('statusSelect').value;

            fetch('/update_stay_status', {
                method: 'POST',
                body: `id=${userId}&status=${encodeURIComponent(status)}`
            }).then(res => res.text()).then(data => {
                if (data === 'Updated') {
                    alert("✅ Status updated successfully!");
                    closeStatusModal();
                    fetchResidenceData();
                } else {
                    alert("❌ Error updating status.");
                }
            });
        });
    }

    // --- 8. LECTURER PORTAL LOGIC ---
    // (Only runs if elements exist, i.e., on lecturer.html)

    function fetchBroadcasts() {
        fetch('/get_broadcasts')
            .then(res => res.text())
            .then(data => {
                // Prefer existing container with id 'broadcast-list'
                let output = document.getElementById('broadcast-list');

                // If not present, try lecturer communication section, then student section
                if (!output) {
                    const lecturerSection = document.getElementById('communication-section');
                    const studentSection = document.getElementById('student-communication-section');

                    if (lecturerSection) {
                        const target = lecturerSection.querySelector('.recent-alerts') || lecturerSection;
                        output = document.createElement('div');
                        output.id = 'broadcast-list';
                        output.style.marginTop = "20px";
                        target.appendChild(output);
                    } else if (studentSection) {
                        const target = studentSection.querySelector('.recent-alerts') || studentSection;
                        output = document.createElement('div');
                        output.id = 'broadcast-list';
                        output.style.marginTop = "10px";
                        target.appendChild(output);
                    }
                }

                if (output) {
                    // Render broadcasts as read-only list for students; lecturers still get delete buttons
                    output.innerHTML = "<h3 style=\"margin-top:0\">Sent Broadcasts</h3>";
                    const rows = data.split(';');
                    rows.forEach(r => {
                        if (!r) return;
                        const [id, to, msg, time, by] = r.split('|');
                        const div = document.createElement('div');
                        div.className = "box";
                        div.style.marginBottom = "10px";
                        // If the page contains the lecturer communication form, show delete button; otherwise read-only
                        const isLecturerView = !!document.getElementById('broadcastForm') || !!document.getElementById('communication-section');
                        div.innerHTML = `
                                <div class="right-side" style="width:100%">
                                    <div class="box-topic">To: ${to} <small style="float:right">${time}</small></div>
                                    <p>${msg}</p>
                                    ${isLecturerView ? `<button onclick="deleteBroadcast('${id}')" style="background:#e74c3c; color:white; border:none; padding:5px; border-radius:3px; cursor:pointer; margin-top:5px;">Delete</button>` : ''}
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

    // --- LECTURER: ENHANCED STUDENT STATUS (Warden Style) ---
    function fetchClassStudents() {
        const tbody = document.getElementById('lecturer-status-tbody');
        const classSelect = document.getElementById('lecEvacClass');

        // 如果页面上没有这个表格，说明当前可能不是 Lecturer 视图，直接返回
        if (!tbody) {
            console.log("Error: lecturer-status-tbody not found");
            return;
        }

        console.log("Fetching class students...");

        // 获取当前筛选的班级 (如果后端支持 filtering，可以在 url 加 query，这里暂时前端过滤或获取全部)
        const selectedClass = classSelect ? classSelect.value : "";
        // 注意：目前 C++ 后端 GetClassStudents 接受 class 参数，你可以根据需要拼接 URL
        // const url = selectedClass ? `/get_students?class=${encodeURIComponent(selectedClass)}` : '/get_students';
        // 暂时保持原样调用，获取所有分配的学生
        const url = '/get_students';

        fetch(url).then(res => {
            if (!res.ok) {
                throw new Error(`HTTP error! status: ${res.status}`);
            }
            return res.text();
        }).then(data => {
            console.log("Student data received (raw):", JSON.stringify(data));
            tbody.innerHTML = "";

            if (!data || data.trim() === "") {
                console.warn("No student data received from server");
                tbody.innerHTML = "<tr><td colspan='5' style='text-align:center; padding: 20px;'>No students found or server not responding</td></tr>";
                return;
            }

            const rows = data.split(';');
            console.log(`Data split into ${rows.length} rows`);

            let safeCount = 0;
            let missingCount = 0;
            let recordsAdded = 0;

            rows.forEach((r, idx) => {
                if (!r || r.trim() === "") {
                    console.log(`Skipping empty row ${idx}`);
                    return;
                }

                console.log(`Processing row ${idx}:`, r);
                // C++ 返回格式: code|name|loc|status
                const parts = r.split('|');
                if (parts.length < 4) {
                    console.warn(`Row ${idx} doesn't have enough parts (${parts.length}):`, r);
                    return;
                }

                const [id, name, loc, status] = parts;

                if (!id || !name) {
                    console.warn(`Row ${idx} has missing critical fields:`, { id, name, loc, status });
                    return;
                }

                // 简单的过滤逻辑 (如果 C++ 没做筛选，前端可以选做)
                // if (selectedClass && !loc.includes(selectedClass)) return; 

                // 1. 统计人数
                if (status === 'Safe') safeCount++;
                else missingCount++;

                // 2. 生成下拉菜单 (Action Column)
                // 使用与 Warden 相同的 updateStudentStatus 逻辑
                const actionHtml = `
                <select onchange="updateStudentStatus('${id}', this.value)" class="status-select">
                    <option value="Safe" ${status === 'Safe' ? 'selected' : ''}>Safe</option>
                    <option value="Missing" ${status === 'Missing' ? 'selected' : ''}>Missing</option>
                    <option value="Injured" ${status === 'Injured' ? 'selected' : ''}>Injured</option>
                </select>
            `;

                const tr = document.createElement('tr');
                tr.innerHTML = `
                <td>${id}</td>
                <td>${name}</td>
                <td>${loc}</td>
                <td><span class="badge ${status === 'Safe' ? 'success' : (status === 'Missing' ? 'danger' : 'warning')}">${status}</span></td>
                <td>${actionHtml}</td>
            `;
                tbody.appendChild(tr);
                recordsAdded++;
            });

            console.log(`Students loaded: ${recordsAdded} records added, ${safeCount} safe, ${missingCount} missing`);

            // 3. 更新统计数字
            const missingEl = document.getElementById('lec-evac-count-missing');
            const safeEl = document.getElementById('lec-evac-count-safe');
            if (missingEl) missingEl.innerText = missingCount;
            if (safeEl) safeEl.innerText = safeCount;
        }).catch(err => {
            console.error("Error fetching students:", err);
            alert("❌ Failed to load students. Check the server console.");
        });

        // 绑定筛选事件 (只绑定一次)
        if (classSelect && !classSelect.dataset.listener) {
            classSelect.addEventListener('change', () => fetchClassStudents());
            classSelect.dataset.listener = "true";
        }
    }

    // 确保更新函数存在 (和 Warden 共用同一个 API)
    window.updateStudentStatus = function (studentId, newStatus) {
        if (!newStatus || !studentId) return;

        console.log(`Updating student ${studentId} to status: ${newStatus}...`);

        if (newStatus === 'Safe') {
            // Use /mark_safe endpoint for Safe status
            fetch('/mark_safe', {
                method: 'POST',
                body: `id=${studentId}`
            })
                .then(res => res.text())
                .then(data => {
                    console.log(`Mark safe response: "${data}"`);
                    if (data && data.includes('Updated')) {
                        console.log(`✅ Student ${studentId} marked as safe`);
                        setTimeout(() => fetchClassStudents(), 300);
                    } else if (data && data.includes('Fail')) {
                        alert(`❌ Failed to mark student as safe`);
                    } else {
                        console.log(`Marking safe completed with response: ${data}`);
                        setTimeout(() => fetchClassStudents(), 300);
                    }
                })
                .catch(err => {
                    console.error('Error marking safe:', err);
                    alert(`❌ Error: ${err.message}`);
                });
        } else {
            // For Missing and Injured, we would need a backend endpoint
            // For now, show a message to the user
            alert(`⚠️ Status "${newStatus}" update is partially supported.\nCurrently, only "Safe" status can be updated directly.\n\nPlease update "${newStatus}" status through the server admin interface.`);
            // Optionally reload to reflect current data
            setTimeout(() => fetchClassStudents(), 500);
        }
    };
    const broadcastForm = document.getElementById('broadcastForm');
    if (broadcastForm) {
        broadcastForm.onsubmit = function (e) {
            e.preventDefault();
            const to = this.querySelector('select').value;
            const msg = this.querySelector('textarea').value;
            const urgency = this.querySelector('input[name="urgency"]:checked')?.value || 'info';

            fetch('/broadcast', {
                method: 'POST',
                body: `to=${encodeURIComponent(to)}&message=${encodeURIComponent(msg)}&urgency=${encodeURIComponent(urgency)}`
            }).then(() => {
                alert("Broadcast Sent!");
                this.reset();
                fetchBroadcasts();
            });
        };
        // Initial load
        fetchBroadcasts();
    }

    // Initialize lecturer-specific functions if page loads correctly
    console.log("Page loaded, initializing...");

    // Try to fetch class students (function will check if elements exist)
    setTimeout(() => {
        console.log("Calling fetchClassStudents from initialization");
        fetchClassStudents();
        console.log("Calling fetchBroadcasts from initialization");
        fetchBroadcasts();
    }, 100);

    setTimeout(initMiniMap, 500);
});