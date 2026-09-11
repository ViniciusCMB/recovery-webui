import styled from "styled-components";

import type { WebSocketData } from "@components/Main/Main";

interface RenderSocketProps {
  data: WebSocketData[];
}

export default function RenderSocket({ data }: RenderSocketProps) {
  let cardContent: React.JSX.Element[] | undefined;
  if (data.length > 0) {
    const lastData = data[data.length - 1];
    cardContent = Object.keys(lastData).map((key) => (
      <Data key={key}>
        {key}: {lastData[key].valueOf()}
      </Data>
    ));
  }

  return <Wrapper>{cardContent}</Wrapper>;
}

const Wrapper = styled.div`
  display: flex;
  flex-direction: column;
  align-items: flex-start;
  gap: 10px;
  align-self: stretch;
  grid-row: span 1;
  grid-column: span 4;
  justify-self: stretch;
  width: max-content;

  @media (min-width: 768px) {
    grid-row: 1 / span 2;
    grid-column: 6 / span 3;
  }

  @media (min-width: 1280px) {
    grid-row: 1 / span 2;
    grid-column: 9 / span 4;
  }
`;

const Data = styled.p`
  color: #fff;
  font-family: Jersey10;
  font-size: 36px;
  margin: 0;
`;
