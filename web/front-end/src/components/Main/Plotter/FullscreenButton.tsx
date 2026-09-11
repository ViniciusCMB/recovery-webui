import styled from "styled-components";

export default styled.div<{ $src: string; $isFullscreen: boolean }>`
  width: 34px;
  aspect-ratio: 1/1;
  user-select: none;
  border: 2px solid rgba(0, 0, 0, 0.2);
  border-radius: 4px;
  cursor: pointer;

  background-image: url("${({ $src }) => $src}");
  background-size: 26px 52px;
  background-repeat: no-repeat;
  background-position-x: center;
  background-position-y: ${({ $isFullscreen }) => ($isFullscreen ? "-22px" : "4px")};
  background-color: white;
  background-origin: border-box;
`;
