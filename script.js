document.addEventListener('DOMContentLoaded', () => {

    // --- 1. ROLE-BASED UI INITIALIZATION ---
    // Check the name injected by C++ to determine if we show lecturer features
    const adminName = document.querySelector('.admin_name').innerText;
    const isLecturer = adminName.includes("Dr.") || adminName.toLowerCase().includes("lecturer");

    if (isLecturer) {
        // Show lecturer-specific sidebar links
        document.querySelectorAll('.lecturer-only').forEach(el => el.style.display = 'block');
        // Show the security/approval panel for incidents
        const securityPanel = document.getElementById('security-panel');
        if (securityPanel) securityPanel.style.display = 'block';
    }

    // --- 2. MAPS & INCIDENTS ---
    let miniMap = null;
    let mainMap = null;
    let markers = [];

    initMiniMap();
    fetchIncidents();
    setInterval(fetchIncidents, 5000); // Live updates every 5s

    function fetchIncidents() {
        fetch('/get_incidents')
            .then(res => res.text())
            .then(data => {
                const pendingTbody = document.getElementById('pending-tbody');
                const activeTbody = document.getElementById('active-tbody');
                if(!activeTbody) return;

                pendingTbody.innerHTML = "";
                activeTbody.innerHTML = "";
                markers.forEach(m => mainMap && mainMap.removeLayer(m));
                markers = [];

                const rows = data.split(';');
                rows.forEach(row => {
                    if(!row.trim()) return;
                    const cols = row.split('|');
                    const [id, type, loc, time, status, lat, lng] = cols;

                    if (status === 'Approved') {
                        const tr = document.createElement('tr');
                        tr.innerHTML = `<td><span class="badge warning">${type}</span></td><td>${loc}</td><td>${time}</td><td><span class="badge info">Official</span></td>`;
                        activeTbody.appendChild(tr);

                        if (mainMap) {
                            const m = L.marker([parseFloat(lat), parseFloat(lng)]).addTo(mainMap)
                                .bindPopup(`<b>${type}</b><br>${loc}`);
                            markers.push(m);
                        }
                    } 
                    else if (status === 'Pending' && isLecturer) {
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
                });
            });
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

            // Specialized handling for maps when switching to the incident tab
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