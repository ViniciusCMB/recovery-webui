import styled from "styled-components";

import type { SelectedMission } from "@components/Main/Main";

interface MissionSelectorProps {
  selectedMission: SelectedMission;
  setSelectedMission: React.Dispatch<React.SetStateAction<SelectedMission>>;
}

export default function MissionSelector({
  selectedMission,
  setSelectedMission,
}: MissionSelectorProps) {
  function handleChange(
    e: React.ChangeEvent<HTMLSelectElement, HTMLSelectElement>,
  ) {
    setSelectedMission(e.target.value as SelectedMission);
  }

  return (
    <Wrapper>
      <label htmlFor="mission-select">Missões:</label>
      <Select
        value={selectedMission}
        name="mission-select"
        onChange={(e) => handleChange(e)}
      >
        <option value="">Selecione uma missão:</option>
        <option value="#11">#11 Dédalo</option>
        <option value="#51">#51 Thonyan</option>
        <option value="#213">#213 Helike</option>
      </Select>
    </Wrapper>
  );
}

const Wrapper = styled.div`
  display: flex;
  flex-direction: column;
  width: fit-content;
  margin-block: 30px 0;
  margin-inline: auto;

  font-family: Jersey10;
  font-size: 40px;
  color: white;
`;

const Select = styled.select`
  font-family: Jersey10;
  font-size: 40px;
  color: #68228b;
`;
