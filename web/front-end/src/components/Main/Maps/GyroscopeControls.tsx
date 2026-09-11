import type { CompassRequestStatus } from "@hooks/useDeviceCompass";
import { useEffect } from "react";
import Control from "react-leaflet-custom-control";
import styled from "styled-components";

import gyroscopeIconSrc from "@components/Main/Maps/gyroscope.svg";

interface GyroscopeControlProps {
  compassRequestStatus: CompassRequestStatus;
  setCompassRequestStatus: React.Dispatch<
    React.SetStateAction<CompassRequestStatus>
  >;
}

interface DeviceOrientationEventConstructor {
  prototype: DeviceOrientationEvent;
  new (
    type: string,
    eventInitDict?: DeviceOrientationEventInit,
  ): DeviceOrientationEvent;
  requestPermission?(): Promise<"granted" | "denied">;
}

function GyroscopeControl({
  compassRequestStatus,
  setCompassRequestStatus,
}: GyroscopeControlProps) {
  const CompassEvent = window.DeviceOrientationEvent as
    DeviceOrientationEventConstructor | undefined;

  useEffect(() => {
    if (!CompassEvent) {
      setCompassRequestStatus("denied");
    } else if (!CompassEvent.requestPermission) {
      setCompassRequestStatus("granted");
    }
  }, [CompassEvent, setCompassRequestStatus]);

  function handleClick() {
    if (!CompassEvent?.requestPermission) return;

    CompassEvent.requestPermission().then((orientationPermission) => {
      setCompassRequestStatus(orientationPermission);
    });
  }

  return (
    <Control prepend position="topleft">
      <Button
        className="leaflet-bar"
        onClick={() => handleClick()}
        $src={gyroscopeIconSrc}
        $isDisplayed={
          CompassEvent &&
          CompassEvent.requestPermission &&
          compassRequestStatus === "idle"
        }
      ></Button>
    </Control>
  );
}

const Button = styled.div<{ $isDisplayed?: boolean; $src?: string }>`
  color: "inherit";
  aspect-ratio: 1/1;
  width: 34px;
  justify-content: center;
  align-items: center;
  padding: 2px;
  display: ${({ $isDisplayed }) => ($isDisplayed ? "grid" : "none")};
  background-image: url("${({ $src }) => $src}");
  background-repeat: no-repeat;
  background-position: center;
  background-size: contain;
  background-color: white;
  cursor: pointer;
`;

export default GyroscopeControl;
