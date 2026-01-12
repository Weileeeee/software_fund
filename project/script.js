document.addEventListener('DOMContentLoaded', () => {
    
    // VARIABLES
    const serverRole = window.SERVER_INJECTED_ROLE || 'student';
    let currentUserRole = (serverRole.toLowerCase() === 'security' || serverRole.toLowerCase() === 'warden') ? 'security' : 'student';
    
    // UI Elements
    const sidebar = document.querySelector(".sidebar");
    const sidebarBtn = document.querySelector(".sidebarBtn");
    const navLinks = document.querySelectorAll(".nav-link");
    const tabContents = document.querySelectorAll(".tab-content");
    const securityPanel = document.getElementById("security-panel");
    const wardenPanel = document.getElementById("warden-panel");
    const reportBox = document.getElementById("report-box");
    
    const reportBtn = document.getElementById("openModalBtn");
    const dashReportBtn = document.getElementById("dashReportBtn");
    const iAmSafeBtn = document.getElementById("iAmSafeBtn");
    const startEvacBtn = document.getElementById("startEvacBtn");
    const genReportBtn = document.getElementById("genReportBtn");
    const evacBlock = document.getElementById("evacBlock");
    
    const modal = document.getElementById("reportModal");
    const closeBtn = document.querySelector(".close-btn");
    const incidentForm = document.getElementById("incidentForm");
    
    const pendingTbody = document.getElementById("pending-tbody");
    const activeTbody = document.getElementById("active-tbody");
    const evacuationTbody = document.getElementById("evacuation-tbody");
    
    let map, miniMap, markers = [];

    // UI INIT
    const isAdmin = (currentUserRole === 'security');

    if(isAdmin) {
        if(securityPanel) securityPanel.style.display = "block";
        if(wardenPanel) wardenPanel.style.display = "block";
        if(reportBtn) reportBtn.style.display = "none";
        if(reportBox) reportBox.style.display = "none";
        if(iAmSafeBtn) iAmSafeBtn.style.display = "none";
    } else {
        if(securityPanel) securityPanel.style.display = "none";
        if(wardenPanel) wardenPanel.style.display = "none";
        if(reportBtn) reportBtn.style.display = "block";
        if(reportBox) reportBox.style.display = "flex";
        // iAmSafeBtn visibility handled below
    }

    if(sidebarBtn) sidebarBtn.onclick = () => sidebar.classList.toggle("active");

    // TABS
    navLinks.forEach(link => {
        link.addEventListener("click", (e) => {
            e.preventDefault();
            navLinks.forEach(l => l.classList.remove("active"));
            tabContents.forEach(tc => tc.style.display = "none");
            link.classList.add("active");
            const targetId = link.getAttribute("data-target");
            document.getElementById(targetId).style.display = "block";
            
            if(targetId === "incidents-section") setTimeout(initMainMap, 200);
            if(targetId === "dashboard-section") setTimeout(initMiniMap, 200);
            if(targetId === "evacuation-section") fetchEvacuationData();
        });
    });

    // MAPS
    function initMainMap() {
        if (!map) {
            map = L.map('main-map').setView([2.9276, 101.6413], 16);
            L.tileLayer('https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png').addTo(map);
            fetchIncidents();
        } else map.invalidateSize();
    }
    
    function initMiniMap() {
        if (!miniMap) {
            miniMap = L.map('mini-map', {zoomControl:false, dragging:false}).setView([2.9276, 101.6413], 15);
            L.tileLayer('https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png').addTo(miniMap);
        } else miniMap.invalidateSize();
    }

    function fetchIncidents() {
        fetch('/get_incidents').then(res => res.text()).then(data => {
            if(!activeTbody) return;
            activeTbody.innerHTML = ""; if(pendingTbody) pendingTbody.innerHTML = "";
            markers.forEach(m => map.removeLayer(m)); markers = [];
            
            data.split(';').forEach(row => {
                if(!row.trim()) return;
                const [id, type, loc, time, status, lat, lng] = row.split('|');
                
                if (status === 'Approved') {
                    const tr = document.createElement('tr');
                    tr.innerHTML = `<td>${type}</td><td>${loc}</td><td>${time}</td><td><span class="badge approved">Active</span></td>`;
                    activeTbody.appendChild(tr);
                    if(map) {
                        const m = L.marker([parseFloat(lat), parseFloat(lng)]).addTo(map).bindPopup(`<b>${type}</b><br>${loc}`);
                        markers.push(m);
                    }
                } else if (status === 'Pending' && isAdmin) {
                    const tr = document.createElement('tr');
                    tr.innerHTML = `<td>${type}</td><td>${loc}</td><td>${time}</td><td><button class="approve-btn" onclick="updateStatus(${id},'approve')">Approve</button></td>`;
                    pendingTbody.appendChild(tr);
                }
            });
        });
    }

    // EVACUATION
    function fetchEvacuationData() {
        fetch('/get_evacuation').then(res => res.text()).then(data => {
            if(!evacuationTbody) return;
            evacuationTbody.innerHTML = "";
            let missing = 0, safe = 0;
            let amIMissing = false;
            const myName = document.querySelector('.admin_name').innerText.replace('Welcome, ', '');

            data.split(';').forEach(row => {
                if(!row.trim()) return;
                const [id, name, block, status, time] = row.split('|');
                if(status === 'Safe') safe++; else missing++;
                
                if(name === myName && status === 'Missing') amIMissing = true;

                let btn = (isAdmin && status === 'Missing') ? `<button class="approve-btn" onclick="markSafe(${id})">Mark Safe</button>` : "-";
                let badge = status === 'Safe' ? '<span class="badge approved">Safe</span>' : '<span class="badge warning">Missing</span>';
                
                const tr = document.createElement('tr');
                tr.innerHTML = `<td>${name}</td><td>${block}</td><td>${badge}</td><td>${time}</td><td>${btn}</td>`;
                evacuationTbody.appendChild(tr);
            });
            document.getElementById('count-missing').innerText = missing;
            document.getElementById('count-safe').innerText = safe;

            if(!isAdmin && iAmSafeBtn) {
                iAmSafeBtn.style.display = amIMissing ? "block" : "none";
            }
        });
    }

    // ACTIONS
    const showModal = () => { modal.style.display = "flex"; }
    if(reportBtn) reportBtn.onclick = showModal;
    if(dashReportBtn) dashReportBtn.onclick = showModal;
    if(closeBtn) closeBtn.onclick = () => modal.style.display = "none";
    
    if(iAmSafeBtn) iAmSafeBtn.onclick = () => { 
        if(confirm("Confirm you are safe?")) fetch('/self_safe', {method:'POST'}).then(()=>fetchEvacuationData()); 
    };

    if(startEvacBtn) startEvacBtn.onclick = () => {
        const blk = evacBlock.value;
        if(confirm(`START EVACUATION for Block ${blk}?`)) fetch('/start_evacuation', {method:'POST', body:`block=${blk}`}).then(()=>fetchEvacuationData());
    };
    if(genReportBtn) genReportBtn.onclick = () => {
        fetch('/generate_report', {method:'POST'}).then(()=>alert("Report Generated!"));
    };

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
                alert("Report Sent!");
                fetchIncidents();
            });
        };
    }

    window.updateStatus = (id, action) => fetch('/update_incident', {method:'POST', body:`id=${id}&action=${action}`}).then(()=>fetchIncidents());
    window.markSafe = (id) => fetch('/mark_safe', {method:'POST', body:`id=${id}`}).then(()=>fetchEvacuationData());
    
    setTimeout(initMiniMap, 500);
});