# Front End do recovery-ui

Esse subprojeto conta com os arquivos de desenvolvimento do front end do projeto
recovery-ui. As dependências do projeto estão listadas em `packages.json`, e
no momento de escrita a versão de Node usada para o projeto é a v24.16.0.

> [!ATTENTION]
> Caso esse texto fique desatualizado, a versão de Node a ser utilizada para
> desenvolvimento deve ser a presente no arquivo `flake.lock` de dependências do
> nix.

> [!TIP]
> Se você for um usuário de nix, é possível fazer cache de seu shell de
> desenvolvimento com `nix-direnv`. Apenas digite `direnv allow` e as
> dependências nix estarão disponíveis ao navegar a esse diretório.

## Contribua

Para desenvolver o front-end é necessário ter `nix` instalado com o recurso
experimental `flake` habilitado.

1. Como qualquer flake, insira `nix develop` em seu prompt para criar seu shell de desenvolvimento.
2. Ao fim você deve ter Node instalado. Cheque com `node -v`.
3. Finalmente, instale as dependências do Node com `npm install`.

Para abrir o servidor de desenvolvimento: `npm run dev`.
Para abrir e export o servidor de desenvolvimento para sua LAN (bom para testar
o layout em outros dispositivos): `npm run dev`.

Para exportar o bundle estático que eventualmente será servido para o back end
flask, use `npm run build`.

E é isso c:
