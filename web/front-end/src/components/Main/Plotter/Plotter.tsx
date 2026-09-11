import Card from "@/shared/Cards/Card";
import { normal } from "color-blend";
import { useEffect, useState } from "react";
import {
  CartesianGrid,
  Legend,
  Line,
  Tooltip,
  XAxis,
  YAxis,
  createHorizontalChart,
} from "recharts";
import styled from "styled-components";

import type { WebSocketData } from "@components/Main/Main";

import FullscreenButton from "./FullscreenButton";
import GraphsFilterMenu from "./GraphFilter";
import IconSrc from "./icon-fullscreen.svg";

interface PlotterProps {
  data: WebSocketData[];
}

export interface CurveProperties {
  [key: string]: {
    isActive: boolean;
    color: string;
  };
}

export default function Plotter({ data }: PlotterProps) {
  const [isFullscreen, setIsFullscreen] = useState(false);
  const [curveProperties, setCurveProperties] = useState<CurveProperties>({});

  const curvePropertiesLength = Object.keys(curveProperties).length;

  let dataKeys: string[] = [];
  if (data.length > 0) {
    dataKeys = Object.keys(data[0]);
  }

  useEffect(() => {
    if (curvePropertiesLength > 0) {
      return;
    }

    const orange = { r: 249, g: 166, b: 0, a: 1 };
    const purple = { r: 104, g: 34, b: 139, a: 1 };
    const blendingScaleFactor = 1 / dataKeys.length;
    let currentBlendingScale = 0;

    setCurveProperties(
      dataKeys.reduce((prev, cur) => {
        purple.a = currentBlendingScale;
        orange.a = 1 - currentBlendingScale;
        currentBlendingScale += blendingScaleFactor;
        const nc = normal(orange, purple);
        return {
          ...prev,
          [cur]: {
            isActive: true,
            color: `rgba(${nc.r}, ${nc.g}, ${nc.b}, ${nc.a})`,
          },
        };
      }, {}),
    );
  }, [data]);

  const Typed = createHorizontalChart<WebSocketData, string, number>()({
    Line,
    XAxis,
    YAxis,
  });

  function handleButtonClick() {
    setIsFullscreen(!isFullscreen);
  }

  const cardContent = (
    <>
      <Typed.LineChart
        style={{
          width: "100%",
          height: isFullscreen ? "80%" : "100%",
        }}
        margin={{ right: 40, bottom: 10, top: 10 }}
        responsive
        data={isFullscreen ? data.slice(-100) : data.slice(-20)}
      >
        <CartesianGrid />
        <Typed.XAxis dataKey="time" />
        <Typed.YAxis />
        <Typed.YAxis />
        {curvePropertiesLength > 0 &&
          dataKeys
            .filter((key) => curveProperties[key].isActive)
            .map((key) => {
              if (key === "time") return;
              return (
                <Typed.Line
                  key={key}
                  isAnimationActive={false}
                  dataKey={key}
                  stroke={curveProperties[key].color}
                />
              );
            })}
        <Legend />
        <Tooltip isAnimationActive={false} />
      </Typed.LineChart>
      <Controls>
        <FullscreenButton
          $src={IconSrc}
          $isFullscreen={isFullscreen}
          onClick={handleButtonClick}
        />
        <GraphsFilterMenu
          filter={curveProperties}
          setFilter={setCurveProperties}
        />
      </Controls>
    </>
  );

  return (
    <Wrapper isFullscreen={isFullscreen} title="Plotter">
      {cardContent}
    </Wrapper>
  );
}

const Wrapper = styled(Card)`
  grid-row: 2 / span 1;
  grid-column: 1 / span 4;

  @media (min-width: 768px) {
    grid-row: 2 / span 1;
    grid-column: 1 / span 5;
  }

  @media (min-width: 1280px) {
    grid-row: 2 / span 1;
    grid-column: 1 / span 8;
  }
`;

const Controls = styled.div`
  display: flex;
  justify-content: center;
  align-items: center;
`;
