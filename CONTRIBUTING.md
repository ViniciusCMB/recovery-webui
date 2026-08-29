# Contribuindo para o Recovery WebUI

Obrigado pelo seu interesse em contribuir! Este documento fornece diretrizes para contribuir com o projeto.

## Código de Conduta

### Nossa Promessa

Nós, como membros, contribuidores e mantenedores, nos comprometemos a tornar a participação em nosso projeto uma experiência livre de assédio para todos.

### Comportamentos Esperados

- Use linguagem acolhedora e inclusiva
- Respeite pontos de vista e experiências diferentes
- Aceite críticas construtivas com elegância
- Foque no que é melhor para a comunidade
- Mostre empatia com outros membros

### Comportamentos Inaceitáveis

- Uso de linguagem ou imagens sexualizadas
- Comentários insultuosos ou depreciativos
- Assédio público ou privado
- Publicação de informações privadas de terceiros
- Conduta não profissional

## Como Contribuir

### Reportando Bugs

Antes de criar um bug report:

1. ✅ Verifique se já não existe uma issue similar
2. ✅ Use a versão mais recente
3. ✅ Tente reproduzir o bug

**Template de Bug Report:**

```markdown
**Descrição do Bug**
Descrição clara e concisa do problema.

**Passos para Reproduzir**

1. Execute '...'
2. Clique em '...'
3. Veja o erro

**Comportamento Esperado**
Descrição do que deveria acontecer.

**Screenshots**
Se aplicável, adicione screenshots.

**Ambiente:**

- OS: [e.g. Ubuntu 22.04]
- Python: [e.g. 3.10.0]
- Browser: [e.g. Chrome 120]
- Módulo LoRa: [e.g. SX1278]

**Logs**

[Cole logs relevantes aqui]

**Informações Adicionais**
Qualquer outro contexto sobre o problema.
```

### Sugerindo Melhorias

**Template de Feature Request:**

```markdown
**A funcionalidade está relacionada a um problema?**
Descrição clara do problema. Ex: "É frustrante quando [...]"

**Descreva a solução desejada**
Descrição clara do que você quer que aconteça.

**Descreva alternativas consideradas**
Outras soluções ou funcionalidades que você considerou.

**Contexto adicional**
Screenshots, mockups, ou outro contexto.
```

### Pull Requests

#### Processo

1. **Fork** o repositório
2. **Clone** seu fork:

   ```bash
   git clone https://github.com/seu-usuario/recovery-webui.git
   cd recovery-webui
   ```

3. **Crie uma branch** para sua feature:

   ```bash
   git checkout -b feature/minha-feature
   ```

4. **Configure o ambiente**:

   ```bash
   python3 -m venv venv
   source venv/bin/activate
   pip install -r requirements.txt
   ```

5. **Faça suas mudanças**

6. **Teste suas mudanças**:

   ```bash
   # Teste manual
   cd app
   python3 app.py

   # Teste automatizado (se disponível)
   pytest
   ```

7. **Commit suas mudanças**:

   ```bash
   git add .
   git commit -m "Adiciona funcionalidade X"
   ```

8. **Push para seu fork**:

   ```bash
   git push origin feature/minha-feature
   ```

9. **Abra um Pull Request** no GitHub

#### Checklist do Pull Request

Antes de submeter, verifique:

- [ ] O código segue os padrões do projeto
- [ ] Comentários foram adicionados em código complexo
- [ ] Documentação foi atualizada (se necessário)
- [ ] Não há console.logs ou prints de debug desnecessários
- [ ] Código foi testado manualmente
- [ ] Commits têm mensagens descritivas
- [ ] Branch está atualizada com main/master

#### Revisão de Código

Seu PR será revisado por mantenedores. Eles podem:

- Aprovar e fazer merge
- Solicitar mudanças
- Pedir esclarecimentos
- Sugerir melhorias

Seja receptivo ao feedback!

## Padrões de Código

### Python

Seguimos [PEP 8](https://pep8.org/):

**Boas práticas:**

```python
# ✓ Bom
def calculate_altitude(pressure: float, reference_pressure: float = 1013.25) -> float:
    """
    Calcula altitude baseada na pressão atmosférica.

    Args:
        pressure: Pressão atual em hPa
        reference_pressure: Pressão de referência (nível do mar)

    Returns:
        Altitude em metros
    """
    altitude = 44330 * (1 - (pressure / reference_pressure) ** 0.1903)
    return round(altitude, 2)

# ✗ Ruim
def calcAlt(p,ref=1013.25):
    return 44330*(1-(p/ref)**0.1903)
```

**Formatação:**

```bash
# Use Black para formatação automática
black app/

# Use Flake8 para linting
flake8 app/
```

### JavaScript

**Boas práticas:**

```javascript
// ✓ Bom
function addMarkerToMap(latitude, longitude, timestamp) {
  if (!latitude || !longitude) {
    console.error("Coordenadas inválidas");
    return;
  }

  const marker = L.marker([latitude, longitude]);
  marker.bindPopup(`Posição em ${timestamp}`);
  layerGroup.addLayer(marker);
}

// ✗ Ruim
function add(lat, lon, t) {
  var m = L.marker([lat, lon]);
  m.bindPopup("Posição em " + t);
  layerGroup.addLayer(m);
}
```

**Convenções:**

- Use `const` e `let`, evite `var`
- Use template strings: `` `valor: ${var}` ``
- Use arrow functions quando apropriado
- Adicione comentários em lógica complexa

### HTML/CSS

```html
<!-- ✓ Bom -->
<div class="telemetry-container">
  <h2 class="section-title">Dados de Telemetria</h2>
  <table class="data-table">
    <thead>
      <tr>
        <th>Hora</th>
        <th>Latitude</th>
        <th>Longitude</th>
      </tr>
    </thead>
    <tbody id="data-body">
      <!-- Dados serão inseridos aqui -->
    </tbody>
  </table>
</div>

<!-- ✗ Ruim -->
<div class="tc">
  <h2>Dados de Telemetria</h2>
  <table>
    <tr>
      <th>Hora</th>
      <th>Latitude</th>
      <th>Longitude</th>
    </tr>
    <!-- dados -->
  </table>
</div>
```

## Convenções de Commit

### Formato

```
<tipo>: <descrição curta>

[corpo opcional]

[rodapé opcional]
```

### Tipos

- **feat**: Nova funcionalidade
- **fix**: Correção de bug
- **docs**: Mudanças na documentação
- **style**: Formatação (não afeta funcionalidade)
- **refactor**: Refatoração de código
- **test**: Adiciona ou modifica testes
- **chore**: Tarefas de manutenção

### Exemplos

```bash
# Bom ✓
git commit -m "feat: adiciona validação de coordenadas GPS"
git commit -m "fix: corrige parse CSV com dados incompletos"
git commit -m "docs: atualiza guia de instalação"
git commit -m "refactor: extrai lógica de parsing para função separada"

# Ruim ✗
git commit -m "changes"
git commit -m "fix stuff"
git commit -m "update"
```

### Corpo do Commit (opcional)

```bash
git commit -m "feat: adiciona gráfico de altitude em tempo real

Implementa Chart.js para visualização de altitude vs tempo.
Dados são atualizados automaticamente via WebSocket.
Gráfico pode ser resetado com botão na interface.

Closes #42"
```

## Estrutura de Branches

### Branches Principais

- **main** / **master**: Código estável em produção
- **develop**: Branch de desenvolvimento (se usado)

### Branches de Feature

```bash
# Novas funcionalidades
feature/nome-da-feature
feature/graficos-altitude
feature/exportar-kml

# Correções de bugs
fix/nome-do-bug
fix/serial-timeout
fix/mapa-nao-centraliza

# Documentação
docs/nome-da-doc
docs/tutorial-raspberry-pi
docs/traducao-pt-br
```

## Testes

### Testes Manuais

**Checklist básico:**

- [ ] Aplicação inicia sem erros
- [ ] Navegador abre automaticamente
- [ ] Conexão WebSocket estabelecida
- [ ] Dados aparecem no mapa
- [ ] Dados aparecem na tabela
- [ ] Log CSV é criado
- [ ] Diferentes TEAM_IDs são diferenciados
- [ ] Navegação entre páginas funciona
- [ ] Funciona em diferentes navegadores

### Testes Automatizados

```python
# tests/test_serial.py
import pytest
from modules.SerialCOM import BaseCom, list_ports

def test_list_ports():
    """Testa listagem de portas."""
    ports = list_ports()
    assert isinstance(ports, list)

def test_base_com_invalid_port():
    """Testa erro com porta inválida."""
    with pytest.raises(Exception):
        com = BaseCom(port='/dev/invalid_port')
```

Execute com:

```bash
pytest tests/
```

## Documentação

### Atualizando Documentação

Ao adicionar funcionalidades:

1. ✅ Atualize o README se necessário
2. ✅ Adicione/atualize comentários no código
3. ✅ Atualize documentação em `docs/` se relevante
4. ✅ Adicione exemplos de uso

### Escrevendo Documentação

**Dicas:**

- Use linguagem clara e concisa
- Adicione exemplos práticos
- Inclua screenshots quando útil
- Mantenha formatação consistente
- Use Markdown corretamente

**Template para documentar função:**

```python
def process_telemetry(data: str) -> dict:
    """
    Processa string de telemetria e retorna dicionário estruturado.

    Args:
        data: String CSV com dados de telemetria no formato:
              "TEAM_ID,millis,count,altp,temp,..."

    Returns:
        dict: Dicionário com campos parseados:
            {
                'team_id': str,
                'millis': int,
                'count': int,
                ...
            }

    Raises:
        ValueError: Se o formato do CSV for inválido

    Example:
        >>> data = "#100,1000,1,150.5,25.3,..."
        >>> result = process_telemetry(data)
        >>> print(result['team_id'])
        '#100'
    """
    # implementação...
```

## Prioridades de Desenvolvimento

### Alta Prioridade

- Correções de bugs críticos
- Segurança
- Performance
- Documentação essencial

### Média Prioridade

- Novas funcionalidades solicitadas
- Melhorias de usabilidade
- Otimizações
- Testes automatizados

### Baixa Prioridade

- Refatorações não urgentes
- Documentação adicional
- Features experimentais

## Licenciamento

Ao contribuir, você concorda que suas contribuições serão licenciadas sob a mesma licença do projeto.

## Reconhecimento

Contribuidores serão listados no README e terão nosso sincero agradecimento! 🎉

### Hall of Contributors

Agradecimentos especiais a todos que contribuíram:

<!-- Será preenchido conforme contribuições -->

## Recursos para Contribuidores

### Documentação

- [Guia de Desenvolvimento](development.md)
- [Arquitetura do Sistema](architecture.md)
- [Documentação da API](api.md)

### Ferramentas Recomendadas

- **Editor**: VS Code, PyCharm, Vim
- **Git GUI**: GitKraken, SourceTree, GitHub Desktop
- **Terminal**: iTerm2 (Mac), Windows Terminal, Terminator (Linux)
- **API Testing**: Postman, Insomnia
- **Browser DevTools**: Chrome DevTools, Firefox Developer Tools

### Tutoriais

- [Git Basics](https://git-scm.com/book/en/v2)
- [Python Style Guide](https://pep8.org/)
- [Flask Documentation](https://flask.palletsprojects.com/)
- [JavaScript MDN](https://developer.mozilla.org/en-US/docs/Web/JavaScript)

## Perguntas?

- 📧 Abra uma [issue](../../issues) com a tag `question`
- 💬 Participe das [discussions](../../discussions)
- 📚 Consulte a [documentação](README.md)

---

## Agradecimentos

Obrigado por considerar contribuir para o Recovery WebUI! Cada contribuição, grande ou pequena, é valiosa. 🚀

**Bons códigos!**

---

[← Voltar à Documentação](README.md) | [Início Rápido →](development.md)
