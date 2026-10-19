<!--
SPDX-License-Identifier: CC-BY-4.0
Adaptado da política de IA do projeto FreeCAD (SPDX-FileCopyrightText: 2026 FreeCAD Developers, CC-BY-4.0).
-->

# Política de IA do FreeCAD Sework

O FreeCAD Sework usa ferramentas de inteligência artificial como **assistentes** no desenvolvimento: para
escrever e revisar código, traduzir textos e rodar testes. Esta política define as regras para que isso aconteça
com responsabilidade e transparência, sempre com **pessoas no comando**.

Esperamos de quem contribui:

- contribuições que a pessoa entende por completo e que revisou e testou;
- declaração do uso de IA;
- comentários e descrições escritos pela própria pessoa.

## Pessoas no comando

Toda contribuição tem uma pessoa responsável por ela, que sabe como o código funciona, responde pelas
consequências e conduz a discussão com os revisores. A IA pode sugerir, escrever rascunhos e executar tarefas,
mas a decisão de aceitar, ajustar ou descartar o resultado é sempre humana.

Não aceitamos PRs com saída de IA sem revisão, nem issues, comentários ou respostas a revisores escritos por IA
no lugar da pessoa.

## Cuidados com a IA

Há preocupações reais em torno da IA, e queremos que quem contribui esteja ciente delas:

- **direitos autorais:** tanto na origem dos dados de treinamento quanto no que a IA produz; quem contribui
  responde pelo que envia;
- **qualidade:** código gerado pode parecer correto e estar errado; por isso os testes e a revisão são
  obrigatórios;
- **ambiente:** o uso de energia e água dos grandes centros de dados;
- **projetos abertos:** excesso de contribuições de baixa qualidade e sobrecarga de quem revisa.

## Medidas concretas

### Declaração

Informe na descrição do PR, em linguagem natural, se e como a IA foi usada. Nos commits, use a linha final
(*trailer*) do Git:

```
Assisted-by: [Família do modelo] ([Versão/ID])
```

ou, quando a ferramenta registra coautoria:

```
Co-Authored-By: [Nome do modelo] <endereço>
```

### Modelo de PR

O modelo de pull request traz uma caixa de confirmação obrigatória: "Este PR não é saída de IA sem
verificação, assumo a responsabilidade por ele e toda a comunicação da minha parte neste PR é feita por mim."

## Contribuições enviadas ao FreeCAD original

Correções enviadas ao [repositório do FreeCAD](https://github.com/FreeCAD/FreeCAD) seguem a
[política de IA do FreeCAD](https://github.com/FreeCAD/FreeCAD/blob/main/AI_POLICY.md), que é mais restritiva
que esta. Leia antes de enviar.


Esta política é licenciada sob [CC BY 4.0](https://creativecommons.org/licenses/by/4.0/) e foi adaptada, com
modificações, da [política de IA do FreeCAD](https://github.com/FreeCAD/FreeCAD/blob/main/AI_POLICY.md).
