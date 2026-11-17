# MovPanels (plugin FFGL para Resolume)

Efeito que divide a imagem em paineis (retos ou em arco) que se movem de forma independente.

## Como gerar o .dll (sem instalar nada)
1. Crie uma conta gratis em https://github.com
2. Crie um repositorio novo (botao "New", nome `movpanels`, pode ser privado).
3. Clique em "uploading an existing file" e arraste TUDO desta pasta
   (as pastas `MovPanels` e `.github`, mais este arquivo). Clique em "Commit changes".
   Importante: a pasta `.github` precisa subir junto (se o Windows esconder, ative "Itens ocultos").
4. Abra a aba **Actions**, espere a execucao "Build MovPanels (Windows)" ficar verde (2 a 5 min).
5. Clique na execucao > em **Artifacts** baixe `MovPanels-dll` (vem num .zip).
6. Extraia `MovPanels.dll` para `Documentos\Resolume Arena\Extra Effects`.
7. Feche o Resolume por completo, abra de novo, e busque por "Mov Panels" na aba Effects.

## Parametros
Panels, Gap, Arch (liga/desliga), Arch Height, Edge Softness,
Content (imagem inteira por painel / fatia continua),
Motion (Up, Down, Alternate, Wave, Horizontal), Speed, Stagger,
Manual Position, Zoom, Mirror Edges, Background (cor e alfa).
