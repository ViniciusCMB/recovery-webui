import styled from "styled-components";

import Logo from "@components/Header/Logo";

function Header() {
  return (
    <Wrapper>
      <Logo />
      <HeaderText>CONTROLE DE MISSÃO</HeaderText>
    </Wrapper>
  );
}

const Wrapper = styled.header`
  display: flex;
  gap: 5px;
  padding-block: 10px;
  padding-inline: 20px;
  justify-content: space-between;
  align-items: center;
  background-color: var(--superficie-invertida, #fff);

  @media (min-width: 768px) {
    padding-block: 15px;
    min-height: 80px;
  }

  @media (min-width: 1280px) {
    position: relative;
    justify-content: center;
    padding: 0;
    min-height: 80px;
  }
`;

const HeaderText = styled.span`
  color: var(--texto-marca, #68228b);
  font-family: Iceland;
  font-size: 30px;
  text-align: right;

  @media (min-width: 768px) {
    font-size: 40px;
  }

  @media (min-width: 1280px) {
    font-size: 48px;
  }
`;

export default Header;
