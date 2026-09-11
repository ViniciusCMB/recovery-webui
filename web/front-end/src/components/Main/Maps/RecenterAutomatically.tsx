import type { LatLngBoundsExpression } from "leaflet";
import { useEffect } from "react";
import { useMap } from "react-leaflet";

interface RecenterAutomaticallyProps {
  bounds: LatLngBoundsExpression;
}

function RecenterAutomatically({ bounds }: RecenterAutomaticallyProps) {
  const map = useMap();

  useEffect(() => {
    map.fitBounds(bounds);
  }, [bounds, map]);
  return null;
}

export default RecenterAutomatically;
