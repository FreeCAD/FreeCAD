# SPDX-License-Identifier: LGPL-2.1-or-later
"""Tradução dos valores de listas (enumerações) do editor de propriedades.

O FreeCAD Sework compilado (fork) pede a tradução de cada valor com o contexto
"App::PropertyEnumeration"; o valor gravado no arquivo continua o original em inglês,
então os projetos abrem normalmente em qualquer FreeCAD.
"""

ENUMS = {
    # Extrusão / cavidade / revolução
    "Length": "Dimensão", "TwoLengths": "Duas dimensões", "UpToLast": "Até a última face",
    "UpToFirst": "Até a primeira face", "UpToFace": "Até a face", "UpToShape": "Até a forma",
    "ThroughAll": "Através de tudo", "Dimension": "Dimensão",
    "Angle": "Ângulo", "TwoAngles": "Dois ângulos",
    "One side": "Um lado", "Two sides": "Dois lados", "Symmetric": "Simétrico",
    "Reversed": "Invertido", "Forward": "Para frente",
    # Furo
    "Hole Depth": "Profundidade do furo", "Tapped (DIN76)": "Roscado (DIN 76)",
    "None": "Nenhum", "Counterbore": "Rebaixo", "Countersink": "Escareado",
    "Counterdrill": "Rebaixo cônico", "Flat": "Plana", "Angled": "Cônica",
    "Standard": "Padrão", "Close": "Justo", "Wide": "Folgado", "Normal": "Normal",
    "Loose": "Folgado", "Medium": "Médio", "Fine": "Fino", "Coarse": "Grosso",
    "Right": "Direita", "Left": "Esquerda",
    "Right-handed": "Rosca direita", "Left-handed": "Rosca esquerda",
    # Booleanas
    "Fuse": "União", "Cut": "Subtração", "Common": "Interseção",
    # Chanfro / offset / casca
    "Equal distance": "Distâncias iguais", "Two distances": "Duas distâncias",
    "Distance and Angle": "Distância e ângulo", "Arcs": "Arcos", "Tangent": "Tangente",
    "Intersection": "Interseção", "Skin": "Pele", "Pipe": "Tubo",
    # Varredura / loft
    "Fixed": "Fixo", "Frenet": "Frenet", "Auxiliary": "Auxiliar",
    "Binormal": "Binormal", "Constant": "Constante", "Multisection": "Multisseção",
    "Linear": "Linear", "S-shape": "Forma de S", "Interpolation": "Interpolação",
    "Transformed": "Transformado", "Right corner": "Canto vivo", "Round corner": "Canto arredondado",
    # Padrões
    "Extent": "Extensão total", "Spacing": "Espaçamento", "Fixed count": "Quantidade fixa",
    "Fixed spacing": "Espaçamento fixo", "Fixed count and spacing": "Quantidade e espaçamento",
    # Hélice
    "pitch-height-angle": "passo-altura-ângulo", "pitch-turns-angle": "passo-voltas-ângulo",
    "height-turns-angle": "altura-voltas-ângulo", "height-turns-growth": "altura-voltas-crescimento",
    # Vínculos
    "Synchronized": "Sincronizado", "Frozen": "Congelado", "Detached": "Desvinculado",
    "Disabled": "Desativado", "Enabled": "Ativado", "Mutated": "Modificado",
    "Automatic": "Automático", "Manual": "Manual", "Custom": "Personalizado",
    # Anexação (modos mais usados)
    "Deactivated": "Desativado", "Translate": "Somente translação", "ObjectXY": "Plano XY do objeto",
    "ObjectXZ": "Plano XZ do objeto", "ObjectYZ": "Plano YZ do objeto", "FlatFace": "Face plana",
    "TangentPlane": "Plano tangente", "NormalToEdge": "Normal à aresta", "Concentric": "Concêntrico",
    "ThreePointsPlane": "Plano por três pontos", "ThreePointsNormal": "Normal por três pontos",
    "ObjectOrigin": "Origem do objeto", "CenterOfMass": "Centro de massa", "MidPoint": "Ponto médio",
    "InertialCS": "Eixos de inércia",
    # Vista
    "Flat Lines": "Faces e arestas", "Shaded": "Sombreado", "Wireframe": "Arame", "Points": "Pontos",
    "Dashed": "Tracejada", "Dotted": "Pontilhada", "Dashdot": "Traço e ponto",
    "Two side": "Dois lados", "Shape": "Forma", "BoundBox": "Caixa envolvente",
    "Orthographic": "Ortográfica", "Perspective": "Perspectiva",
    # Desenho técnico
    "Page": "Folha", "Portrait": "Retrato", "Landscape": "Paisagem",
    "First angle": "1º diedro", "Third angle": "3º diedro",
    "Front": "Frontal", "Rear": "Posterior", "Top": "Superior", "Bottom": "Inferior",
    "Filled arrow": "Seta cheia", "Open arrow": "Seta aberta", "Tick": "Traço oblíquo", "Dot": "Ponto",
    "Open circle": "Círculo aberto", "Fork": "Forquilha", "Filled triangle": "Triângulo cheio",
    "Circular": "Circular", "Rectangle": "Retângulo", "Square": "Quadrado", "Hexagon": "Hexágono",
    "Triangle": "Triângulo", "Inspection": "Inspeção",
    "Continuous": "Contínua", "Visible": "Visível", "Hidden": "Oculta", "Section": "Corte",
    "Center": "Centro", "Symmetry": "Simetria", "Extension": "Chamada", "Leader": "Linha de chamada",
    "Phantom": "Fantasma", "Chain": "Traço e ponto", "Bold": "Negrito", "Italic": "Itálico",
    "Bold-Italic": "Negrito e itálico", "Projected": "Projetada", "True": "Verdadeira",
    "Distance": "Distância", "DistanceX": "Distância X", "DistanceY": "Distância Y",
    "Radius": "Raio", "Diameter": "Diâmetro", "Area": "Área",
    # Simulação (FEM)
    "static": "estática", "frequency": "frequências naturais", "thermomech": "termomecânica",
    "check": "verificação da malha", "buckling": "flambagem",
    "1st": "1ª ordem", "2nd": "2ª ordem", "From Shape": "Da forma",
    "VeryCoarse": "Muito grossa", "Moderate": "Média", "VeryFine": "Muito fina",
    "UserDefined": "Definida pelo usuário", "Fluid": "Fluido",
    "Serial": "Série", "Parallel": "Paralelo",
}
