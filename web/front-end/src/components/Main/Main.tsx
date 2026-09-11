import Cards from "@/shared/Cards/Cards";
import socket from "@/websocket/websocket";
import { useEffect, useState } from "react";
import styled from "styled-components";

import Maps from "./Maps/Maps";
import MissionSelector from "./MissionSelector/MissionSelector";
import Plotter from "./Plotter/Plotter";
import RenderSocket from "./RenderSocket";

export type SelectedMission = "" | "#11" | "#51" | "#213";

export interface WebSocketData {
  [key: string]: string | number | Date;
  altura: number;
  latitude: number;
  longitude: number;
  pressao: number;
  rssi: number;
  satelites: number;
  team_id: SelectedMission;
  temperatura: number;
  time: string;
  umidade: number;
}

const DATA_BUFFER_SIZE = 100;

export default function Main() {
  const [data, setData] = useState<WebSocketData[]>([]);
  const [selectedMission, setSelectedMission] = useState<SelectedMission>("");

  useEffect(() => {
    function handleEvent(payload: WebSocketData) {
      console.log(payload)
      setData((prev) => [
        ...prev.slice(-DATA_BUFFER_SIZE),
        {
          altura: Number(payload.altura),
          latitude: Number(payload.latitude),
          longitude: Number(payload.longitude),
          pressao: Number(payload.pressao),
          rssi: Number(payload.rssi),
          satelites: Number(payload.satelites),
          team_id: payload.team_id,
          temperatura: Number(payload.altura),
          time: new Date(payload.time).toTimeString().split(" ")[0],
          umidade: Number(payload.umidade),
        },
      ]);
    }

    socket.on("updateSat", handleEvent);
    socket.on("updateRocket", handleEvent);

    return () => {
      socket.off("updateSat", handleEvent);
      socket.off("updateRocket", handleEvent);
    };
  }, []);

  const filteredData = data.filter(
    (dataPoint) => dataPoint.team_id === selectedMission,
  );

  return (
    <Wrapper>
      <MissionSelector
        selectedMission={selectedMission}
        setSelectedMission={setSelectedMission}
      />
      {selectedMission !== "" && filteredData.length > 0 && (
        <Cards>
          <Maps missionCoords={filteredData} />
          <Plotter data={filteredData} />
          <RenderSocket data={filteredData} />
        </Cards>
      )}
    </Wrapper>
  );
}

const Wrapper = styled.main`
  flex: 1;
  display: flex;
  flex-direction: column;
  background:
    radial-gradient(
      62.14% 67.71% at 90.48% 74.59%,
      rgba(249, 166, 0, 0.2) 0%,
      var(--superficie-secundaria, rgba(104, 34, 139, 0.2)) 100%
    ),
    radial-gradient(
      71.83% 36.1% at 19.56% 13.85%,
      rgba(98, 178, 227, 0.26) 0%,
      var(--superficie-secundaria, rgba(104, 34, 139, 0.26)) 100%
    ),
    var(--superficie-secundaria, #68228b);
`;
