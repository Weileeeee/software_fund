// --- Global functions ---
function fetchStudents() {
    fetch('/get_students')
        .then(res => res.text())
        .then(data => {
            console.log('Students data:', data);
            const tbody = document.querySelector('#student-status-section tbody');
            if (!tbody) return;
            tbody.innerHTML = "";
            const students = data.split(';');
            console.log('Students array:', students);
            students.forEach(s => {
                if (!s.trim()) return;
                const parts = s.split('|');
                const student_id = parts[0];
                const name = parts[1];
                const location = parts[2];
                const status = parts[3].replace(';', '');
                console.log('Parsed student:', student_id, name, location, status);
                const tr = document.createElement('tr');
                tr.innerHTML = `
                    <td>${student_id}</td>
                    <td>${name}</td>
                    <td>${location}</td>
                    <td><span class="badge ${status === 'Safe' ? 'success' : 'danger'}">${status}</span></td>
                    <td><select class="status-select"><option ${status === 'Safe' ? 'selected' : ''}>Mark Safe</option><option ${status === 'Missing' ? 'selected' : ''}>Missing</option></select></td>
                `;
                tbody.appendChild(tr);
            });
            console.log('Students table updated');
        })
        .catch(err => console.error("Error fetching students:", err));
}

function fetchBroadcasts() {
    console.log('Fetching broadcasts');
    fetch('/get_broadcasts')
        .then(res => res.text())
        .then(data => {
            console.log('Broadcast data:', data);
            const dashBroadcasts = document.getElementById('dashboard-broadcasts');
            if (!dashBroadcasts) return;
            dashBroadcasts.innerHTML = "";
            const broadcasts = data.split(';');
            broadcasts.forEach(b => {
                if (!b.trim()) return;
                const [id, to_group, message, sent_at, sent_by] = b.split('|');
                let deleteBtn = '';
                // Note: isLecturer and isAdmin are not available here, so deleteBtn always empty for now
                // To fix, pass role or check in HTML
                dashBroadcasts.innerHTML += `<div data-id="${id}" style="padding: 10px; border: 1px solid #ddd; margin-bottom: 10px; border-radius: 5px;"><strong>To: ${to_group}</strong><br>${decodeURIComponent(message)}<br><small>Sent by: ${sent_by} at ${sent_at}</small>${deleteBtn}</div>`;
            });
        })
        .catch(err => console.error("Error fetching broadcasts:", err));
}

document.addEventListener('DOMContentLoaded', () => {

    // --- 1. ROLE-BASED UI INITIALIZATION ---
    const adminName = document.querySelector('.admin_name').innerText;
    const isLecturer = adminName.includes("Dr.") || adminName.toLowerCase().includes("lecturer");
    const isAdmin = adminName.toLowerCase().includes("admin");

    if (isLecturer || isAdmin) {
        document.querySelectorAll('.lecturer-only').forEach(el => el.style.display = 'block');
        const securityPanel = document.getElementById('security-panel');
        if (securityPanel) securityPanel.style.display = 'block';
    }

    // --- 2. MAPS & INCIDENTS ---
    let miniMap = null;
    let mainMap = null;
    let markers = [];

    initMiniMap();
    fetchIncidents();
    fetchBroadcasts();
    fetchStudents();
    setInterval(fetchIncidents, 5000); // Live updates every 5s

    function fetchIncidents() {
        fetch('/get_incidents')
            .then(res => res.text())
            .then(data => {
                const pendingTbody = document.getElementById('pending-tbody');
                const activeTbody = document.getElementById('active-tbody');
                if(!activeTbody) return;

                // Clear previous rows and markers
                pendingTbody.innerHTML = "";
                activeTbody.innerHTML = "";
                markers.forEach(m => mainMap && mainMap.removeLayer(m));
                markers = [];

                const rows = data.split(';');
                rows.forEach(row => {
                    if(!row.trim()) return;
                    const cols = row.split('|');
                    const [id, type, loc, time, status, lat, lng] = cols;

                    // --- 1. Pending incidents for lecturers and admins ---
                    if ((status === 'Pending' || status === 'Reported') && (isLecturer || isAdmin)) {
                        const tr = document.createElement('tr');
                        tr.innerHTML = `
                            <td><span class="badge warning">${type}</span></td>
                            <td>${loc}</td>
                            <td>${time}</td>
                            <td>
                                <button class="action-btn btn-approve" onclick="updateStatus(${id}, 'approve')">Approve</button>
                                <button class="action-btn btn-reject" onclick="updateStatus(${id}, 'reject')">Reject</button>
                            </td>
                        `;
                        pendingTbody.appendChild(tr);
                    }

                    // --- 2. Active incidents for everyone ---
                    // Show all Approved incidents and Pending/Reported for everyone
                    if (status === 'Approved' || status === 'Pending' || status === 'Reported') {
                        const tr = document.createElement('tr');
                        tr.innerHTML = `
                            <td><span class="badge warning">${type}</span></td>
                            <td>${loc}</td>
                            <td>${time}</td>
                            <td><span class="badge ${status === 'Approved' ? 'info' : 'warning'}">${status === 'Reported' ? 'Pending' : status}</span></td>
                        `;
                        activeTbody.appendChild(tr);

                        // Add marker to map
                        if (mainMap) {
                            const m = L.marker([parseFloat(lat), parseFloat(lng)]).addTo(mainMap)
                                .bindPopup(`<b>${type}</b><br>${loc}<br><span>Status: ${status === 'Reported' ? 'Pending' : status}</span>`);
                            markers.push(m);
                        }
                    }
                });
            })
            .catch(err => console.error("Error fetching incidents:", err));
    }

    // --- 3. UI TAB NAVIGATION ---
    const navLinks = document.querySelectorAll('.nav-link');
    const sections = document.querySelectorAll('.tab-content');

    navLinks.forEach(link => {
        link.addEventListener('click', (e) => {
            e.preventDefault();
            const targetId = link.getAttribute('data-target');
            if(!targetId) return;

            navLinks.forEach(l => l.classList.remove('active'));
            sections.forEach(s => s.classList.remove('active'));

            link.classList.add('active');
            document.getElementById(targetId).classList.add('active');

            // Initialize main map when switching to incidents
            if(targetId === 'incidents-section') setTimeout(initMainMap, 200);
        });
    });

    // --- 4. FORM HANDLERS ---
    const incidentForm = document.getElementById('incidentForm');
    const modal = document.getElementById('reportModal');
    const openModalBtn = document.getElementById('openModalBtn');
    const closeBtn = document.querySelector('.close-btn');

    if(openModalBtn) openModalBtn.onclick = () => modal.style.display = "flex";
    if(closeBtn) closeBtn.onclick = () => modal.style.display = "none";

    if(incidentForm) {
        incidentForm.onsubmit = (e) => {
            e.preventDefault();
            const type = document.getElementById('inType').value;
            const loc = document.getElementById('inLoc').value;
            const time = document.getElementById('inTime').value;
            const lat = 2.9276 + (Math.random() * 0.005 - 0.0025);
            const lng = 101.6413 + (Math.random() * 0.005 - 0.0025);

            fetch('/report_incident', {
                method: 'POST',
                body: `type=${type}&location=${loc}&time=${time}&lat=${lat}&lng=${lng}`
            }).then(() => {
                modal.style.display = "none";
                incidentForm.reset();
                alert("Report Sent for Approval.");
                fetchIncidents();
            });
        };
    }

    // --- 5. MAP INITIALIZATION ---
    function initMiniMap() {
        const miniMapEl = document.getElementById('mini-map');
        if (!miniMap && miniMapEl) {
            miniMap = L.map('mini-map', {zoomControl:false, dragging:false}).setView([2.9276, 101.6413], 15);
            L.tileLayer('https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png').addTo(miniMap);
        }
    }

    function initMainMap() {
        const mainMapEl = document.getElementById('main-map');
        if (!mainMap && mainMapEl) {
            mainMap = L.map('main-map').setView([2.9276, 101.6413], 15);
            L.tileLayer('https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png').addTo(mainMap);
            fetchIncidents();
        } else if (mainMap) {
            mainMap.invalidateSize();
        }
    }

});

// --- 6. Helper function for lecturer actions (global) ---
function updateStatus(id, action) {
    fetch(`/update_incident?id=${id}&action=${action}`, {method: 'POST'})
        .then(() => alert(`Incident ${action}d.`))
        .then(() => {
            document.querySelectorAll('.nav-link.active')[0].click(); // refresh tab
        });
}

// --- 7. Student status update ---
const submitBtn = document.querySelector('#student-status-section .btn');
if (submitBtn) {
    submitBtn.addEventListener('click', () => {
        const rows = document.querySelectorAll('#student-status-section tbody tr');
        const updates = [];
        rows.forEach(row => {
            const id = row.cells[0].innerText;
            const select = row.querySelector('.status-select');
            const status = select.value;
            updates.push(`student_id=${encodeURIComponent(id)}&status=${encodeURIComponent(status === 'Mark Safe' ? 'Safe' : 'Missing')}`);
        });
        // For simplicity, update one by one or batch
        Promise.all(updates.map(update => fetch('/update_student_status', { method: 'POST', body: update }).then(res => {
            if (!res.ok) throw new Error('Update failed for ' + update);
            return res;
        })))
            .then(() => {
                alert('All student statuses updated!');
                fetchStudents(); // Refresh
            })
            .catch(err => {
                console.error('Update failed:', err);
                alert('Update failed. Check console.');
            });
    });
}

// --- 8. Delete broadcast ---
function deleteBroadcast(id) {
    if (confirm('Are you sure you want to delete this broadcast?')) {
        fetch('/delete_broadcast', { method: 'POST', body: `id=${id}` })
            .then(() => {
                // Remove from dashboard
                console.log('Removing div for id:', id);
                const div = document.querySelector(`#dashboard-broadcasts div[data-id="${id}"]`);
                if (div) {
                    console.log('Found div, removing');
                    div.remove();
                } else {
                    console.log('Div not found');
                }
                alert('Broadcast deleted.');
            })
            .catch(err => {
                console.error('Delete failed:', err);
                alert('Delete failed.');
            });
    }
}
