import styled from "styled-components";

export default function Cards({ children }: CardsProps) {
  return <S_Cards>{children}</S_Cards>;
}

interface CardsProps {
  children?: React.ReactNode;
}

const S_Cards = styled.div`
  display: inline-grid;
  width: 100%;
  padding: 20px;
  row-gap: 15px;
  align-self: stretch;
  grid-template-rows: repeat(3, fit-content(100%));
  grid-template-columns: repeat(4, minmax(0, 1fr));
  grid-auto-flow: row;

  @media (min-width: 768px) {
    display: grid;
    grid-template-rows: repeat(2, minmax(0, 1fr));
    grid-template-columns: repeat(8, minmax(0, 1fr));
    column-gap: 20px;
    row-gap: 10px;
    margin-inline: auto;
    padding: 40px;
    max-width: 660px;
  }

  @media (min-width: 1280px) {
    grid-template-rows: repeat(2, fit-content(100%));
    grid-template-columns: repeat(12, minmax(0, 1fr));
    max-width: 1000px;
  }
`;
