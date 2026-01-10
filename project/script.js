document.addEventListener('DOMContentLoaded', () => {

    // --- VARIABLES ---
    let currentUserRole = "student"; // Default
    // In a real app, you'd check a hidden field for the role, 
    // but here we toggle it via the sidebar/login simulation.

    // Map Vars
    let miniMap = null;
    let mainMap = null;
    let markers = []; // Store markers to clear them later

    // UI Elements
    const pendingTbody = document.getElementById('pending-tbody');
    const activeTbody = document.getElementById('active-tbody');
    const incidentForm = document.getElementById('incidentForm');
    const modal = document.getElementById('reportModal');
    
    // --- 1. INITIALIZE & FETCH DATA ---
    initMiniMap();
    fetchIncidents(); // Load data from DB immediately

    // Poll for updates every 5 seconds (Live interaction!)
    setInterval(fetchIncidents, 5000);

    // --- 2. FETCH FUNCTION ---
    function fetchIncidents() {
        fetch('/get_incidents')
            .then(res => res.text())
            .then(data => {
                // Clear current tables and map
                pendingTbody.innerHTML = "";
                activeTbody.innerHTML = "";
                // Clear existing map markers
                markers.forEach(m => mainMap.removeLayer(m));
                markers = [];

                // Parse CSV Data: 1|Fire|Block A|10:00|Pending|2.9|101.6;
                const rows = data.split(';');
                
                rows.forEach(row => {
                    if(!row.trim()) return;
                    const cols = row.split('|');
                    const id = cols[0];
                    const type = cols[1];
                    const loc = cols[2];
                    const time = cols[3];
                    const status = cols[4];
                    const lat = parseFloat(cols[5]);
                    const lng = parseFloat(cols[6]);

                    // LOGIC: WHERE TO SHOW THIS INCIDENT?
                    
                    if (status === 'Approved') {
                        // 1. SHOW ON ACTIVE TABLE (For everyone)
                        const tr = document.createElement('tr');
                        tr.innerHTML = `<td><span class="badge warning">${type}</span></td><td>${loc}</td><td>${time}</td><td><span class="badge info">Official</span></td>`;
                        activeTbody.appendChild(tr);

                        // 2. SHOW ON MAP (For everyone)
                        if (mainMap) {
                            const m = L.marker([lat, lng]).addTo(mainMap)
                                .bindPopup(`<b>${type}</b><br>${loc}<br><i>Official Alert</i>`);
                            markers.push(m);
                        }
                    } 
                    else if (status === 'Pending') {
                        // 3. SHOW ON PENDING TABLE (Only for Security View)
                        // Note: In this demo, we show it if the security panel is visible
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

    // --- 3. SUBMIT INCIDENT (Student) ---
    if(incidentForm) {
        incidentForm.onsubmit = (e) => {
            e.preventDefault();
            const type = document.getElementById('inType').value;
            const loc = document.getElementById('inLoc').value;
            const time = document.getElementById('inTime').value;

            // Generate Random Coord near MMU Cyberjaya for demo
            // In real life, you'd pick this from the map click
            const lat = 2.9276 + (Math.random() * 0.005 - 0.0025);
            const lng = 101.6413 + (Math.random() * 0.005 - 0.0025);

            fetch('/report_incident', {
                method: 'POST',
                body: `type=${type}&location=${loc}&time=${time}&lat=${lat}&lng=${lng}`
            }).then(() => {
                modal.style.display = "none";
                incidentForm.reset();
                alert("Report Sent! It will appear on the map once a Security Officer approves it.");
                fetchIncidents(); // Refresh UI
            });
        };
    }

    // --- 4. UPDATE STATUS (Security) ---
    window.updateStatus = function(id, action) {
        fetch('/update_incident', {
            method: 'POST',
            body: `id=${id}&action=${action}`
        }).then(() => {
            alert("Incident Updated.");
            fetchIncidents(); // Refresh UI to move item from Pending to Active/Map
        });
    };

    // --- UI & TABS ---
    const navLinks = document.querySelectorAll('.nav-link');
    const sections = document.querySelectorAll('.tab-content');
    const openModalBtn = document.getElementById('openModalBtn');
    const closeBtn = document.querySelector('.close-btn');

    navLinks.forEach(link => {
        link.addEventListener('click', (e) => {
            e.preventDefault();
            navLinks.forEach(l => l.classList.remove('active'));
            sections.forEach(s => s.classList.remove('active'));
            link.classList.add('active');
            const target = document.getElementById(link.getAttribute('data-target'));
            target.classList.add('active');
            if(link.getAttribute('data-target') === 'incidents-section') setTimeout(initMainMap, 200);
        });
    });

    if(openModalBtn) openModalBtn.onclick = () => modal.style.display = "flex";
    if(closeBtn) closeBtn.onclick = () => modal.style.display = "none";
    window.onclick = (e) => { if (e.target == modal) modal.style.display = "none"; }

    // --- MAPS ---
    function initMiniMap() {
        if (!miniMap && document.getElementById('mini-map')) {
            miniMap = L.map('mini-map', {zoomControl:false, dragging:false}).setView([2.9276, 101.6413], 15);
            L.tileLayer('https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png').addTo(miniMap);
        }
    }
    function initMainMap() {
        if (!mainMap && document.getElementById('main-map')) {
            mainMap = L.map('main-map').setView([2.9276, 101.6413], 15);
            L.tileLayer('https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png').addTo(mainMap);
            fetchIncidents(); // Load markers onto the newly created map
        } else if (mainMap) {
            mainMap.invalidateSize();
        }
    }
});