import styled from "styled-components";

import type { CurveProperties } from "./Plotter";
import bulletIconSrc from "./rocket.svg";

interface GraphsFilterMenuProps {
  filter: CurveProperties;
  setFilter: React.Dispatch<React.SetStateAction<CurveProperties>>;
}

export default function GraphsFilterMenu({
  filter,
  setFilter,
}: GraphsFilterMenuProps) {
  function handleChange(key: string) {
    setFilter((prev) => {
      return {
        ...prev,
        [key]: {
          ...prev[key],
          isActive: !prev[key].isActive,
        },
      };
    });
  }

  return (
    <List $bulletIconSrc={bulletIconSrc}>
      {Object.entries(filter).map(([key, value]) => {
        if (key === "time") return;
        return (
          <li key={key}>
            <span>{key}</span>
            <input
              type="checkbox"
              checked={value.isActive}
              onChange={() => handleChange(key)}
            />
          </li>
        );
      })}
    </List>
  );
}

const List = styled.ul<{ $bulletIconSrc: string }>`
  list-style: url("${({ $bulletIconSrc }) => $bulletIconSrc}");
`;
