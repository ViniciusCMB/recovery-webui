import useDeviceCompass from "@/customHooks/useDeviceCompass";
import Card from "@/shared/Cards/Card";
import L from "leaflet";
import "leaflet/dist/leaflet.css";
import { useEffect, useState } from "react";
import { MapContainer, Marker, Popup, TileLayer } from "react-leaflet";
import { FullscreenControl } from "react-leaflet-fullscreen";
import "react-leaflet-fullscreen/styles.css";
import styled from "styled-components";

import type { WebSocketData } from "@components/Main/Main";
import GyroscopeControl from "@components/Main/Maps/GyroscopeControls";
import customIconSrc from "@components/Main/Maps/marker.svg";

type CoordsString = Pick<WebSocketData, "latitude" | "longitude">;
type Coords = Record<keyof CoordsString, number>;

interface MapsProps {
  missionCoords: CoordsString[];
}

export default function Maps({ missionCoords }: MapsProps) {
  const [userCoords, setUserCoords] = useState<Coords>();
  const { compassRequestStatus, setCompassRequestStatus } = useDeviceCompass();

  useEffect(() => {
    const handlerID = navigator.geolocation.watchPosition(
      ({ coords: userCoords }) => {
        setUserCoords({
          latitude: userCoords.latitude,
          longitude: userCoords.longitude,
        });
      },
    );

    return () => {
      navigator.geolocation.clearWatch(handlerID);
    };
  }, []);

  const maxBoundOffset = 0.2;
  const customIcon = new L.Icon({
    iconUrl: customIconSrc,
    iconSize: new L.Point(40, 40),
    iconAnchor: new L.Point(20, 40),
  });

  let cardContent: React.JSX.Element | undefined;
  if (missionCoords.length > 0) {
    const lastCoords = missionCoords[missionCoords.length - 1];
    const { latitude: missionLat, longitude: missionLong } = lastCoords;

    if(!Number.isNan(latitude) && !Number.isNan(longitude)) {
      cardContent = (
        <MapContainer
          center={[missionLat, missionLong]}
          scrollWheelZoom={true}
          zoomControl={false}
          zoom={13}
          minZoom={12}
          maxZoom={16}
          maxBounds={[
            [missionLat - maxBoundOffset, missionLong - maxBoundOffset],
            [missionLat + maxBoundOffset, missionLong + maxBoundOffset],
          ]}
        >
          <TileLayer
            attribution={
              "Tiles &copy; Esri &mdash; Source: Esri, i-cubed, USDA, " +
              "USGS, AEX, GeoEye, Getmapping, Aerogrid, IGN, IGP, UPR-EGP, " +
              "and the GIS User Community"
            }
            url="/ArcGIS/{z}/{x}/{y}.png"
          />
          <Marker position={[missionLat, missionLong]} icon={customIcon}>
            <Popup>Posição da missão</Popup>
          </Marker>
          {userCoords && (
            <Marker
              position={[userCoords.latitude, userCoords.longitude]}
              icon={customIcon}
            >
              <Popup>Você!</Popup>
            </Marker>
          )}
          <FullscreenControl />
          <GyroscopeControl
            compassRequestStatus={compassRequestStatus}
            setCompassRequestStatus={setCompassRequestStatus}
          />
        </MapContainer>
      );
    }

  }

  return <Wrapper title="Coordenadas">{cardContent}</Wrapper>;
}

const Wrapper = styled(Card)`
  grid-row: 1 / span 1;
  grid-column: 1 / span 4;

  @media (min-width: 768px) {
    grid-row: 1 / span 1;
    grid-column: 1 / span 5;
  }

  @media (min-width: 1280px) {
    grid-row: 1 / span 1;
    grid-column: 1 / span 8;
  }
`;
