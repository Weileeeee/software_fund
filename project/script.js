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
                if (activeTbody) activeTbody.innerHTML = "";
                if (pendingTbody) pendingTbody.innerHTML = "";

                markers.forEach(m => map.removeLayer(m));
                markers = [];

                const rows = data.split(';');
                rows.forEach(rowStr => {
                    if (!rowStr) return;
                    const [id, type, loc, time, status, lat, lng] = rowStr.split('|');

                    if (status === 'Approved') {
                        const tr = document.createElement('tr');
                        let actionHtml = `<span class="badge info">View Only</span>`;

                        if (permissions.canManageIncidents) {
                            actionHtml = `<button class="delete-btn" onclick="resolveIncident('${id}')">Resolve</button>`;
                        }

                        tr.innerHTML = `<td>${type}</td><td>${loc}</td><td>${time}</td><td><span class="badge approved">Active</span></td><td>${actionHtml}</td>`;
                        if (activeTbody) activeTbody.appendChild(tr);

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
                        if (pendingTbody) pendingTbody.appendChild(tr);
                    }
                });
            });
    }

    // --- 4. Reporting Logic ---
    const showReportModal = () => {
        modal.style.display = "flex";
        const locInput = document.getElementById('inLoc');
        locInput.value = "📍 Locating...";
        getGPS(locInput);
    };

    const showWardenModal = () => {
        const wModal = document.getElementById('wardenReportModal');
        if (wModal) {
            wModal.style.display = "flex";
            const locInput = document.getElementById('wInLoc');
            locInput.value = "📍 Locating...";
            getGPS(locInput);
        }
    };

    function getGPS(inputElement) {
        if (navigator.geolocation) {
            navigator.geolocation.getCurrentPosition(
                (position) => {
                    reportLat = position.coords.latitude;
                    reportLng = position.coords.longitude;
                    inputElement.value = `📍 GPS (${reportLat.toFixed(4)}, ${reportLng.toFixed(4)})`;
                },
                (error) => {
                    inputElement.value = "";
                    inputElement.placeholder = "Location failed";
                    reportLat = 2.9276; reportLng = 101.6413;
                }
            );
        }
    }

    if (reportBtn) reportBtn.addEventListener('click', showReportModal);
    if (dashReportBtn) dashReportBtn.addEventListener('click', showReportModal);

    // Bind Warden Button to NEW Modal
    const wardenReportBtn = document.getElementById("wardenReportBtn");
    if (wardenReportBtn) wardenReportBtn.addEventListener('click', showWardenModal);

    if (closeBtn) closeBtn.onclick = () => modal.style.display = "none";
    const closeWardenBtn = document.getElementById("closeWardenModal");
    if (closeWardenBtn) closeWardenBtn.onclick = () => document.getElementById('wardenReportModal').style.display = "none";

    // Standard Incident Form (Basic)
    if (incidentForm) {
        incidentForm.onsubmit = (e) => {
            e.preventDefault();
            const type = document.getElementById('inType').value;
            const loc = document.getElementById('inLoc').value;
            const time = document.getElementById('inTime').value;
            // Basic report sends empty details for optional fields
            fetch('/report_incident', {
                method: 'POST',
                body: `type=${type}&location=${loc}&time=${time}&lat=${reportLat}&lng=${reportLng}&desc=Basic Report&severity=Medium&evidence=`
            }).then(res => res.text()).then(id => {
                if (id && id !== "Fail") {
                    modal.style.display = "none";
                    incidentForm.reset();
                    alert("Report Submitted!");
                    fetchIncidents();
                } else alert("Failed.");
            });
        };
    }

    // Warden Incident Form (Detailed)
    const wardenForm = document.getElementById("wardenIncidentForm");
    if (wardenForm) {
        wardenForm.onsubmit = (e) => {
            e.preventDefault();
            const type = document.getElementById('wInType').value;
            const loc = document.getElementById('wInLoc').value;
            const time = document.getElementById('wInTime').value;
            const desc = document.getElementById('wInDesc').value;
            const sev = document.getElementById('wInSev').value;
            const evid = document.getElementById('wInEvid').value;

            fetch('/report_incident', {
                method: 'POST',
                body: `type=${type}&location=${loc}&time=${time}&lat=${reportLat}&lng=${reportLng}&desc=${desc}&severity=${sev}&evidence=${evid}`
            })
                .then(res => res.text())
                .then(id => {
                    if (id && id !== "Fail") {
                        document.getElementById('wardenReportModal').style.display = "none";
                        wardenForm.reset();
                        alert(`Warden Log Saved Successfully! \n\nIncident ID: ${id}`);
                        fetchIncidents();
                    } else {
                        alert("Failed to save log.");
                    }
                });
        };
    }

    // --- 5. Evacuation Logic (Fixed for Hostel/Automark) ---
    function fetchEvacuationData() {
        fetch('/get_evacuation').then(res => res.text()).then(data => {
            if (!evacuationTbody) return;
            evacuationTbody.innerHTML = "";
            let missing = 0, safe = 0;
            let amIMissing = false;

            const welcomeMsg = document.querySelector('.admin_name') ? document.querySelector('.admin_name').innerText : "";
            const myName = welcomeMsg.replace('Welcome, ', '');

            const rows = data.split(';');
            rows.forEach(row => {
                if (!row.trim()) return;
                const [id, name, block, status, time] = row.split('|');

                if (status === 'Safe') safe++; else missing++;
                if (name === myName && status === 'Missing') amIMissing = true;

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

            if (document.getElementById('count-missing')) document.getElementById('count-missing').innerText = missing;
            if (document.getElementById('count-safe')) document.getElementById('count-safe').innerText = safe;

            // Show "I am Safe" only if student is marked 'Missing' by the Warden
            if (permissions.isStudent && iAmSafeBtn) {
                iAmSafeBtn.style.display = amIMissing ? "block" : "none";
            }
        });
    }

    // Fixed: Ensure "ALL" is sent if no block is selected to trigger automark for all hostel students
    if (startEvacBtn) startEvacBtn.onclick = () => {
        if (!permissions.canManageEvacuation) {
            alert("Unauthorized"); return;
        }

        const blkInput = document.getElementById("evacBlock");
        const blk = (blkInput && blkInput.value) ? blkInput.value : "ALL";

        if (confirm(`🚨 EMERGENCY: Start Evacuation for ${blk === "ALL" ? "ALL HOSTELS" : "Block " + blk}? \n\nThis will mark all hostel residents as UNSAFE.`)) {
            fetch('/start_evacuation', {
                method: 'POST',
                body: `block=${blk}`
            })
                .then(res => {
                    if (res.ok) {
                        alert("⚠️ Evacuation Started! All hostel students marked 'Missing'. Students must self-verify safety.");
                        fetchEvacuationData();
                    }
                });
        }
    };

    if (genReportBtn) genReportBtn.onclick = () => {
        if (!permissions.canManageEvacuation) return;
        fetch('/generate_report', { method: 'POST' }).then(() => alert("Report Generated! Check Server."));
    };

    if (iAmSafeBtn) iAmSafeBtn.onclick = () => {
        if (confirm("Confirm you are safe?")) {
            fetch('/self_safe', { method: 'POST' }).then(() => fetchEvacuationData());
        }
    };

    // --- 6. GLOBAL ACTIONS ---
    window.postUpdate = function (id, action) {
        if (!permissions.canManageIncidents) return;
        fetch('/update_incident', {
            method: 'POST',
            body: `id=${id}&action=${action}`
        }).then(res => {
            if (res.ok) {
                alert("Incident " + action + "d!");
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