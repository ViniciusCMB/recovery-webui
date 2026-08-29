// Table references (may not exist on every page)
var table = document.getElementById("table");
if (!table) table = null;

// Inicializa o mapa
var map = L.map(document.getElementById("mapDIV"), {});

// Cria a layer group para os pontos serem adicionados (foguete)
var layerGroup = L.layerGroup();
// Layer group para pontos do satélite (separado)
var satLayerGroup = L.layerGroup();

// Layer do open street map
var basemap = L.tileLayer("http://{s}.tile.osm.org/{z}/{x}/{y}.png", {
  maxZoom: 18,
});
basemap.addTo(map);

// Layer do google sat
var googleSat = L.tileLayer(
  "http://{s}.google.com/vt/lyrs=s&x={x}&y={y}&z={z}",
  {
    maxZoom: 20,
    subdomains: ["mt0", "mt1", "mt2", "mt3"],
  }
);
googleSat.addTo(map);

// Layer do google streets
googleStreets = L.tileLayer(
  "http://{s}.google.com/vt/lyrs=m&x={x}&y={y}&z={z}",
  {
    maxZoom: 20,
    subdomains: ["mt0", "mt1", "mt2", "mt3"],
  }
);
googleStreets.addTo(map);

// Junta todas as layers de mapa em uma variável e as adiciona ao controle do mapa
var basemaps = {
  "Google Satélite": googleSat,
  "Google Streets": googleStreets,
  "Open Streets Map": basemap,
};
// Overlays para alternar visibilidade de pontos (foguete / satélite)
var overlays = {
  "Foguete": layerGroup,
  "Satélite": satLayerGroup,
};

L.control.layers(basemaps, overlays).addTo(map);

// Adiciona uma escala ao mapa
var scale = L.control.scale();
scale.addTo(map);

// Adiciona pontos ao mapa
function MapPoint(latitude, longitude, time) {
  layerGroup.addLayer(
    L.marker([latitude, longitude]).bindPopup(
      latitude + "," + longitude + " em " + time
    )
  );
  layerGroup.addTo(map);
}

function MapSatPoint(latitude, longitude, time) {
  // marcador diferente (circle) para satélite
  satLayerGroup.addLayer(
    L.circleMarker([latitude, longitude], { radius: 6, color: "#d63031" }).bindPopup(
      "Satélite: " + latitude + "," + longitude + " em " + time
    )
  );
  satLayerGroup.addTo(map);
}

// Adiciona dados à tabela e ao mapa
function addData(latitude, longitude, altura, satelites, time, rssi, pqd) {
  addFirstPoint(latitude, longitude);
  MapPoint(latitude, longitude, time);

  var content = "";
  content += "<tr>";
  content += "<td>" + time + "</td>";
  content += "<td>" + latitude + "</td>";
  content += "<td>" + longitude + "</td>";
  content += "<td>" + altura + "</td>";
  content += "<td>" + satelites + "</td>";
  content += "<td>" + rssi + "</td>";
  content += "<td>" + pqd + "</td>";
  content += "</tr>";

  if (table) {
    table.insertAdjacentHTML("afterbegin", content);
  } else {
    console.warn('Rocket table not present on this page; skipping DOM update for rocket data.');
  }
}

// Adiciona dados do satélite à tabela e ao mapa (satLayerGroup)
function addSatData(latitude, longitude, altura, temperatura, umidade, pressao, satelites, time, rssi) {
  addFirstPoint(latitude, longitude);
  MapSatPoint(latitude, longitude, time);

  var content = "";
  content += "<tr>";
  content += "<td>" + time + "</td>";
  content += "<td>" + latitude + "</td>";
  content += "<td>" + longitude + "</td>";
  content += "<td>" + altura + "</td>";
  content += "<td>" + satelites + "</td>";
  content += "<td>" + temperatura + "</td>";
  content += "<td>" + umidade + "</td>";
  content += "<td>" + pressao + "</td>";
  content += "<td>" + rssi + "</td>";
  content += "</tr>";

  var satTable = document.getElementById("satTable");
  if (satTable) {
    satTable.insertAdjacentHTML("afterbegin", content);
  }
}

// Centraliza o mapa no primeiro ponto recebido
var firstPoint = true;
function addFirstPoint(latitude, longitude) {
  if (firstPoint) {
    map.setView([latitude, longitude], 16); // Centraliza no primeiro ponto
    firstPoint = false;
  }
}

// Configuração do socket.io
$(document).ready(function () {
  var socket = io.connect();

  // On page load, fetch historical rocket entries from the server log and populate the table
  if (table) {
    fetch('/api/logs/rocket')
      .then(function (res) {
        if (!res.ok) throw new Error('Failed to fetch rocket logs');
        return res.json();
      })
      .then(function (list) {
        if (!Array.isArray(list)) return;
        // list is chronological (oldest -> newest). addData inserts at the top, so
        // iterating in order will result in the newest entries appearing at the top.
        list.forEach(function (item) {
          try {
            addData(item.latitude, item.longitude, item.altura, item.satelites, item.time, item.rssi, item.pqd);
          } catch (e) {
            console.warn('Failed to add historical row:', e);
          }
        });
      })
      .catch(function (err) {
        console.warn('Could not load rocket logs:', err);
      });
  }

  // Recebe os dados do foguete
  socket.on("updateRocket", function (msg) {
    var jsonData = msg;
    console.log(jsonData);
    var latitude = jsonData.latitude;
    var longitude = jsonData.longitude;
    var satelites = jsonData.satelites;
    var time = jsonData.time;
    var altura = jsonData.altura;
    var rssi = jsonData.rssi;
    var pqd = jsonData.pqd;
    if (pqd == "1") {
      pqd = "Sim";
    }
    else {
      pqd = "Não";
    }
    console.log(
      time +
        " -> " +
        latitude +
        " , " +
        longitude +
        " , " +
        altura +
        " , " +
        satelites
    );
    addData(latitude, longitude, altura, satelites, time, rssi, pqd);
  });

  // Recebe os dados do satélite
  socket.on("updateSat", function (msg) {
    var jsonData = msg;
    console.log("updateSat:", jsonData);
    var latitude = jsonData.latitude;
    var longitude = jsonData.longitude;
    var altura = jsonData.altura;
    var satelites = jsonData.satelites;
    var temperatura = jsonData.temperatura;
    var umidade = jsonData.umidade;
    var pressao = jsonData.pressao;
    var rssi = jsonData.rssi;
    var pqd = jsonData.pqd;
    var time = jsonData.time;

    addSatData(latitude, longitude, altura, temperatura, umidade, pressao, satelites, time, rssi);
  });
});
