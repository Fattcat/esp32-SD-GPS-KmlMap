from fastkml import kml
import folium

# Načítaj KML
with open("track.kml", "r", encoding="utf-8") as f:
    doc = f.read()

k = kml.KML()
k.from_string(doc.encode("utf-8"))

# Hľadanie všetkých súradníc
coords = []
for feature in list(k.features()):
    for placemark in list(feature.features()):
        if placemark.geometry and hasattr(placemark.geometry, 'coords'):
            coords += list(placemark.geometry.coords)

# Ak nie sú dáta
if not coords:
    print("❌ Žiadne súradnice")
    exit()

# Vytvorenie mapy
map_center = [coords[0][1], coords[0][0]]
m = folium.Map(location=map_center, zoom_start=15)

# Zobrazenie trasy
latlon = [(lat, lon) for lon, lat, *_ in coords]
folium.PolyLine(latlon, color="green", weight=3).add_to(m)

# Uloženie mapy
m.save("mapa.html")
print("✅ Mapa uložená do mapa.html")
