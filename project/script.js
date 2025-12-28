document.addEventListener('DOMContentLoaded', () => {
    
    // --- SIDEBAR LOGIC ---
    let sidebar = document.querySelector(".sidebar");
    let sidebarBtn = document.querySelector(".sidebarBtn");
    
    // Toggle Sidebar Class
    if (sidebarBtn) {
        sidebarBtn.onclick = function() {
            sidebar.classList.toggle("active");
            // Important: Trigger map resize if map is visible
            setTimeout(() => {
                if(miniMap) miniMap.invalidateSize();
                if(mainMap) mainMap.invalidateSize();
            }, 300);
        }
    }

    // --- VARIABLES ---
    const navLinks = document.querySelectorAll('.nav-link');
    const sections = document.querySelectorAll('.tab-content');
    
    const openModalBtn = document.getElementById('openModalBtn');
    const modal = document.getElementById('reportModal');
    const closeBtn = document.querySelector('.close-btn');
    const incidentForm = document.getElementById('incidentForm');
    
    const pendingTbody = document.getElementById('pending-tbody');
    const activeTbody = document.getElementById('active-tbody');

    // Map Variables
    let miniMap = null;
    let mainMap = null;

    // --- 1. TAB SWITCHING LOGIC ---
    navLinks.forEach(link => {
        link.addEventListener('click', (e) => {
            e.preventDefault();

            navLinks.forEach(l => l.classList.remove('active'));
            sections.forEach(s => s.classList.remove('active'));

            const targetId = link.getAttribute('data-target');
            link.classList.add('active');
            const targetSection = document.getElementById(targetId);
            if(targetSection) targetSection.classList.add('active');

            // Resize maps when switching tabs
            if(targetId === 'incidents-section') {
                setTimeout(() => initMainMap(), 200);
            } else if(targetId === 'dashboard-section') {
                setTimeout(() => { if(miniMap) miniMap.invalidateSize(); }, 200);
            }
        });
    });

    // --- 2. MODAL LOGIC ---
    if(openModalBtn) openModalBtn.onclick = () => modal.style.display = "flex";
    if(closeBtn) closeBtn.onclick = () => modal.style.display = "none";
    window.onclick = (e) => { if (e.target == modal) modal.style.display = "none"; }

    // --- 3. SUBMIT INCIDENT ---
    if(incidentForm) {
        incidentForm.onsubmit = (e) => {
            e.preventDefault();
            const type = document.getElementById('inType').value;
            const loc = document.getElementById('inLoc').value;
            const time = document.getElementById('inTime').value;
            const rowId = Date.now();

            const tr = document.createElement('tr');
            tr.setAttribute('data-id', rowId);
            tr.innerHTML = `
                <td><span class="badge warning">${type}</span></td>
                <td>${loc}</td>
                <td>${time}</td>
                <td>
                    <button class="action-btn btn-approve" onclick="handleReport(${rowId}, 'approve', '${type}', '${loc}')">Approve</button>
                    <button class="action-btn btn-reject" onclick="handleReport(${rowId}, 'reject')">Reject</button>
                </td>
            `;
            pendingTbody.appendChild(tr);
            modal.style.display = "none";
            incidentForm.reset();
            alert("Report Submitted! Waiting for Security Approval.");
        };
    }

    // --- 4. APPROVE/REJECT LOGIC ---
    window.handleReport = function(id, action, type, loc) {
        const row = document.querySelector(`tr[data-id="${id}"]`);
        if (!row) return;

        if (action === 'approve') {
            const cells = row.querySelectorAll('td');
            const time = cells[2].innerText;
            const activeTr = document.createElement('tr');
            activeTr.innerHTML = `<td><span class="badge warning">${type}</span></td><td>${loc}</td><td>${time}</td><td><span class="badge info">Official</span></td>`;
            activeTbody.appendChild(activeTr);

            if (mainMap) {
                let lat = 2.9276 + (Math.random() * 0.002 - 0.001);
                let lng = 101.6413 + (Math.random() * 0.002 - 0.001);
                L.marker([lat, lng]).addTo(mainMap).bindPopup(`<b>${type}</b><br>${loc}`).openPopup();
            }
            alert("Incident Approved & Added to Map.");
        } 
        row.remove();
    };

    // --- 5. MAP INITIALIZATION ---
    function initMiniMap() {
        if (!miniMap && document.getElementById('mini-map')) {
            miniMap = L.map('mini-map', {
                zoomControl: true,       // ENABLED ZOOM BUTTONS
                dragging: true,          // ENABLED DRAGGING
                scrollWheelZoom: false   // Disabled scroll to allow page scrolling
            }).setView([2.9276, 101.6413], 15);
            
            L.tileLayer('https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png').addTo(miniMap);
        }
    }

    function initMainMap() {
        if (!mainMap && document.getElementById('main-map')) {
            mainMap = L.map('main-map').setView([2.9276, 101.6413], 16);
            L.tileLayer('https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png', { attribution: '© OpenStreetMap' }).addTo(mainMap);
            mainMap.invalidateSize();
        } else if (mainMap) {
            mainMap.invalidateSize();
        }
    }

    initMiniMap();
});