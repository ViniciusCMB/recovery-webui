import { useEffect, useState } from "react";

export type CompassRequestStatus = "idle" | "granted" | "denied";

interface CompassEvent extends DeviceOrientationEvent {
  webkitCompassHeading?: number;
}

export default function useDeviceCompass() {
  const [compassRequestStatus, setCompassRequestStatus] =
    useState<CompassRequestStatus>("idle");
  const [userCompass, setUserCompass] = useState<string>();

  useEffect(() => {
    if (compassRequestStatus !== "granted") return;

    function handleOrientation(e: CompassEvent) {
      if (!e.webkitCompassHeading) return;

      const userCompass = e.webkitCompassHeading.toLocaleString("pt-BR", {
        minimumIntegerDigits: 3,
        maximumFractionDigits: 2,
        minimumFractionDigits: 2,
      });
      setUserCompass(userCompass);
    }

    window.addEventListener("deviceorientation", handleOrientation);

    return () => {
      window.removeEventListener("deviceorientation", handleOrientation);
    };
  }, [compassRequestStatus]);

  return {
    userCompass,
    compassRequestStatus,
    setCompassRequestStatus,
  };
}
