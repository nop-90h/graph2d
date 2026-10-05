precision mediump float;

// Юниформы
uniform sampler2D u_image0, u_image1, u_image2, u_image3;
uniform float u_shockTime, u_lowHPEffect, u_time, u_aberration; 
uniform vec2 u_shockCenter, u_resolution, u_tutRectSize;
uniform vec4 u_tutParams; 


// Юниформы для 5 вспышек
uniform vec3 u_hit0; uniform float u_hitI0;
uniform vec3 u_hit1; uniform float u_hitI1;
uniform vec3 u_hit2; uniform float u_hitI2;
uniform vec3 u_hit3; uniform float u_hitI3;
uniform vec3 u_hit4; uniform float u_hitI4;
uniform vec3 u_hitColor;

// Упакованные Varying
varying vec4 v_v1; // [texCoord.x, texCoord.y, alphaBlend, sparkLife]
varying vec4 v_v2; // tintColor
varying vec4 v_v3; // gsColor
varying vec4 v_v4; // addRGB
varying vec4 v_v5; // texMuls (0, 1, 2, 3)
varying vec2 v_v6; // [effectType, tutIntensity]

void main() {
    // --- 1. РАСПАКОВКА ---
    vec2  uv_main      = v_v1.xy;
    float alphaBlend   = v_v1.z;
    float sparkLife    = v_v1.w;
    
    vec4  tintColor    = v_v2;
    vec4  gsColor      = v_v3;
    vec4  addRGB       = v_v4;
    vec4  texMuls      = v_v5;
    
    float effectType   = v_v6.x;
    float tutIntensity = v_v6.y;

    vec2 uv_distorted = uv_main;

    // --- 2. ИСКАЖЕНИЯ ---
    if (effectType > 6.5 && effectType < 7.5) {
        float p_jitter = fract(sin(dot(uv_distorted, vec2(12.9, 78.2))) * 437.5);
        uv_distorted += (p_jitter - 0.5) * 0.003 * sparkLife;
    }

    if (u_shockTime > 0.0) {
        // Глитч
        if (u_shockTime < 0.2) {
            uv_distorted.x += sin(uv_distorted.y * 40.0) * u_aberration * 0.1;
        }
        // Шоквейв
        vec2 shock_dir = uv_distorted - u_shockCenter;
        float dist = length(shock_dir);
        if (dist < u_shockTime + 0.1) {
            float mask_shock = smoothstep(u_shockTime - 0.1, u_shockTime, dist) * 
                         (1.0 - smoothstep(u_shockTime, u_shockTime + 0.1, dist));
            uv_distorted += normalize(shock_dir + 0.0001) * (mask_shock * 0.05 * (1.0 - u_shockTime));
        }
    }

    vec2 dist_vec = uv_distorted - 0.5;

    // --- 3. ВЫБОРКА С УЧЕТОМ ОТСТУПА ---
    vec4 color_out = vec4(0.0);
    
    // Определяем размер отступа (например, 3-4 пикселя, что соответствует 0.004 для текстуры 256x256)
    const float safe_padding = 0.004;
    
    // Создаем маску: 1.0, если UV-координаты находятся внутри допустимой области с учетом отступа, иначе 0.0
    //float mask = step(safe_padding, uv_distorted.x) * (1.0 - step(1.0 - safe_padding, uv_distorted.x)) * 
    //             step(safe_padding, uv_distorted.y) * (1.0 - step(1.0 - safe_padding, uv_distorted.y));

    if (v_v5.x > 0.5) {
        color_out = texture2D(u_image0, uv_main);
    } else if (v_v5.y > 0.5) {
        color_out = texture2D(u_image1, uv_main);
    } else if (v_v5.z > 0.5) {
        color_out = texture2D(u_image2, uv_main);
    } else if (v_v5.w > 0.5) {
        color_out = texture2D(u_image3, uv_main);
    }

    // Применяем маску к финальному цвету
    //color_out *= mask;


    if (gsColor.a > 0.0) {
        float grey = dot(color_out.rgb, vec3(0.299, 0.587, 0.114));
        color_out.rgb = mix(color_out.rgb, vec3(grey), gsColor.a);
    }

    // --- 3. ЭФФЕКТЫ (Dynamic Branching - считаем только активный) ---
    float lum = clamp(dot(color_out.rgb, vec3(0.333)), 0.0, 1.0);

    if (effectType > 0.5 && effectType < 1.5) { // Sparks
        vec3 spark_rgb = (sin(vec3(0.0, 2.09, 4.18) + (v_v1.x + v_v1.y) * 30.0 - u_time * 6.0) * 0.5 + 0.5) * lum * 1.5 * (0.5 + sparkLife * 0.5);
        color_out.rgb = spark_rgb;
    } 
    else if (effectType > 1.5 && effectType < 2.5) { // Poison Particle
        color_out.rgb = mix(vec3(0.0, 0.35, 0.0), vec3(0.5, 1.0, 0.0) * (sin(u_time * 10.0) * 0.15 + 0.85), clamp(lum * 2.5, 0.0, 1.0));
        color_out.a *= sparkLife;
    }
    else if (effectType > 2.5 && effectType < 3.5) { // Soul
        color_out.rgb = mix(vec3(0.6, 0.1, 1.0), vec3(1.0), 1.0 - lum) * 1.5;
        color_out.a *= sparkLife * 0.4;
    }
    else if (effectType > 5.5 && effectType < 6.5) { // Burn
        vec3 burn_col = mix(vec3(0.8, 0.05, 0.0), vec3(1.0, 0.4, 0.0), lum) * 2.0;
        color_out.rgb = mix(color_out.rgb, burn_col, sparkLife * (1.1 - lum));
    }
    else if (effectType > 6.5 && effectType < 7.5) { // Poison Acid Flow
        float p_flow = sin(uv_main.y * 20.0 + uv_main.x * 10.0 - u_time * 5.0) * 0.5 + 0.5;
        vec3 acid_col = mix(vec3(0.0, 1.0, 0.2), vec3(0.8, 1.0, 0.0), p_flow);
        color_out.rgb = (color_out.rgb * mix(vec3(1.0), vec3(0.4, 0.8, 0.2), sparkLife)) + (acid_col * (1.0 - lum) * p_flow * sparkLife * 1.5);
    }
else if (effectType > 7.5 && effectType < 8.5) { // Holy Gold
    // 1. Раскаляем персонажа до золотого свечения
    vec3 holyGold = vec3(1.0, 0.8, 0.4);
    color_out.rgb = mix(color_out.rgb, holyGold * 2.0, sparkLife * lum);
    
    // 2. Генерируем лучи (Raymarching light)
    float angle = atan(dist_vec.y, dist_vec.x);
    float rayFreq = 12.0; // Количество лучей
    float rays = pow(abs(sin(angle * rayFreq + u_time * 2.0)), 10.0);
    
    // 3. Заставляем лучи мерцать и затухать к краям
    float beam = rays * smoothstep(0.5, 0.1, length(dist_vec));
    
    // 4. Накладываем лучи (они "прожигают" текстуру)
    color_out.rgb += holyGold * beam * 5.0 * sparkLife;
    
    // 5. Вспышка в самом начале (как мы делали в молнии)
    float flash = pow(sparkLife, 15.0);
    color_out.rgb += vec3(1.0, 0.95, 0.8) * flash * 3.0;
}
    else if (effectType > 8.5 && effectType < 9.5) { // Dark Flame 2.0
        // 1. Делаем центр ЧЕРНЫМ (чем выше яркость спрайта, тем темнее цвет)
        // Возводим lum в степень, чтобы "вырезать" середину посильнее
        float core_mask = pow(lum, 2.0); 
        
        // 2. Создаем свечение на КРАЯХ (где текстура начинает исчезать)
        // Это и будет наше "сияние по краям"
        float edge_mask = smoothstep(0.0, 0.4, lum) * (1.0 - core_mask);
        
        // 3. Считаем цвета
        vec3 core_rgb = vec3(0.0); // Абсолютно черное ядро
        vec3 glow_rgb = tintColor.rgb * 3.0; // Яркое сияние (умножаем на 3 для HDR эффекта)
        
        // Смешиваем: в центре черный, по краям - свечение
        color_out.rgb = mix(glow_rgb, core_rgb, core_mask);
        
        // Добавляем самосветящийся ободок (edge_mask) поверх, чтобы края "горели"
        color_out.rgb += glow_rgb * edge_mask * 0.5;
        
        color_out.a *= sparkLife;
    }
else if (effectType > 9.5 && effectType < 10.5) { // Glitch Beast
    float scan = sin(u_time * 10.0) * 0.5 + 0.5;
    // Инвертируем яркость, оставляя только "скелетные" очертания
    float bones = pow(1.0 - lum, 3.0) * color_out.a;
    vec3 boneCol = mix(vec3(0.0, 0.2, 0.5), vec3(0.8, 0.9, 1.0), bones);
    color_out.rgb = mix(color_out.rgb, boneCol * 3.0, sparkLife * scan);
}
else if (effectType > 10.5 && effectType < 11.5) { // Inferno Fringe 2.0
    // 1. Берем исходную маску персонажа
    float baseAlpha = color_out.a;
    
    // 2. Создаем внутренний контур (чтобы пламя не вылезало за меш)
    // Мы не берем пиксели снаружи, мы смотрим, где внутри персонажа "заканчивается плотность"
    float innerEdge = smoothstep(0.0, 0.5, baseAlpha) * (1.0 - smoothstep(0.5, 0.9, baseAlpha));
    
    // 3. Динамический шум пламени (используем только координаты текущего пикселя)
    // Добавляем зависимость от Y, чтобы пламя "сужалось" или "рвалось" кверху
    float noise = sin(uv_main.x * 30.0 + u_time * 15.0) * cos(uv_main.y * 20.0 - u_time * 10.0);
    noise += sin(uv_main.x * 60.0 - u_time * 20.0) * 0.5; // Дополнительная детализация
    
    // 4. Формируем языки пламени, привязанные к альфе
    // Умножаем на baseAlpha, чтобы пламя ГАРАНТИРОВАННО исчезало там, где кончается спрайт
    float flame = smoothstep(0.2, 0.6, innerEdge + noise * 0.4);
    flame *= baseAlpha; 
    
    // 5. Цвет: делаем "горячее" основание
    // Цвета: темно-красный -> оранжевый -> почти белый (HDR)
    vec3 fireCol = mix(vec3(0.8, 0.0, 0.0), vec3(1.0, 0.6, 0.0), flame);
    fireCol = mix(fireCol, vec3(1.0, 1.0, 0.8), pow(flame, 3.0)); 
    
    // Накладываем пламя поверх персонажа (Add/Screen эффект)
    color_out.rgb += fireCol * flame * 4.0 * sparkLife;
    
    // Если хочешь, чтобы персонаж сам немного поджарился (потемнел) под огнем:
    color_out.rgb = mix(color_out.rgb, fireCol * 0.5, flame * sparkLife * 0.5);
}
else if (effectType > 11.5 && effectType < 12.5) { // Lightning Strike
    // 1. Тайминги: дискретное время для "дерганости" и быстрое мерцание
    float burstTime = floor(u_time * 22.0); // Быстрая смена формы
    float flicker = sin(u_time * 80.0) * 0.5 + 0.5; // Сверхбыстрое дрожание яркости
    
    // 2. Фрактальный зигзаг (Jagged Path)
    // Делаем путь молнии ломаным через комбинацию синусов и fract
    float yCoord = uv_main.y * 2.5;
    float zigzag = abs(fract(yCoord + burstTime * 0.1) - 0.5);
    float noise = sin(yCoord * 12.0 + burstTime * 15.0) * 0.12 * zigzag;
    // Добавляем случайный горизонтальный скачок всей молнии
    noise += (fract(sin(burstTime * 12.9898) * 43758.5453) - 0.5) * 0.15;
    
    // 3. Расчет позиции разряда
    float xPos = uv_main.x - 0.5 + noise;
    
    // 4. Маски: Ствол молнии и вторичные дуги
    // Основной луч (сужается к концу жизни sparkLife)
    float beam = smoothstep(0.07 * sparkLife, 0.0, abs(xPos));
    // Ослепительное ядро
    float core = smoothstep(0.02 * sparkLife, 0.0, abs(xPos));
    // Тонкие "волоски" электричества вокруг
    float arcs = smoothstep(0.012, 0.0, abs(xPos + sin(uv_main.y * 45.0 + burstTime) * 0.04));
    
    // 5. Цветовая модель "Электричество"
    vec3 glowCol = vec3(0.2, 0.5, 1.0); // Синий ореол
    vec3 coreCol = vec3(0.8, 0.95, 1.0); // Бело-голубое ядро
    
    // Собираем разряд (аддитивная энергия)
    vec3 lightningRGB = glowCol * beam * 5.0 * flicker; // Мягкий свет
    lightningRGB += coreCol * beam * 10.0;             // Мощный центр
    lightningRGB += coreCol * arcs * 4.0 * flicker;    // Микро-разряды
    
    // Маскируем строго по альфе персонажа, чтобы не было квадратов атласа
    lightningRGB *= color_out.a;
    
    // 6. НАЛОЖЕНИЕ
    // Просто добавляем свет молнии поверх оригинального арта
    // Умножаем на sparkLife, чтобы эффект красиво затухал в конце анимации
    color_out.rgb += lightningRGB * sparkLife;
    
    // 7. Опционально: легкий ореол ионизации по самому краю (внутри альфы)
    float edgeGlow = pow(color_out.a, 4.0) * (1.0 - color_out.a) * 2.0;
    color_out.rgb += glowCol * edgeGlow * flicker * sparkLife;
}
else if (effectType > 12.5 && effectType < 13.5) {
    float drops = step(0.98, fract(uv_main.x * 10.0 + u_time * 2.0 + sin(uv_main.y * 20.0)));
    color_out.rgb = mix(color_out.rgb, vec3(0.6, 0.0, 0.0), sparkLife);
    color_out.rgb += vec3(1.0, 0.0, 0.0) * drops * sparkLife * 5.0;
}
else if (effectType > 13.5 && effectType < 14.5) {
    float angle = atan(dist_vec.y, dist_vec.x);
    float d = length(dist_vec);
    vec2 p = vec2(angle * 0.15, 1.0 / (d + 0.1) + u_time);
    color_out.rgb = mix(color_out.rgb, vec3(0.1, 0.8, 1.0) * (sin(p.y * 10.0) * 0.5 + 0.5), sparkLife);
    color_out.a *= smoothstep(0.0, 0.2 * sparkLife, d);
}
else if (effectType > 14.5 && effectType < 15.5) { // Silk Gold 2.0 (Liquid Metal)
    // 1. Создаем карту высот на основе яркости (lum) и UV
    // Это заставит золото "играть" на складках арта
    float slope = lum + sin(uv_main.x * 10.0 + uv_main.y * 5.0);
    
    // 2. Двойной скользящий блик (Anisotropic Streaks)
    // Один быстрый и узкий, второй широкий и медленный
    float shine1 = sin((uv_main.x + uv_main.y) * 8.0 + u_time * 3.0 + slope * 2.0);
    float shine2 = sin((uv_main.x - uv_main.y) * 5.0 - u_time * 1.5);
    
    // Делаем блики острыми (как на полированном металле)
    float streak1 = pow(max(0.0, shine1), 15.0); 
    float streak2 = pow(max(0.0, shine2), 10.0);
    
    // 3. Цветовая палитра "Deep Gold"
    // Базовый цвет темного золота для теней и яркий для бликов
    vec3 goldDark = vec3(0.4, 0.2, 0.0);
    vec3 goldBright = vec3(1.0, 0.8, 0.3);
    vec3 goldWhite = vec3(1.0, 1.0, 0.8);
    
    // 4. Логика окрашивания
    // Мы не просто красим поверх, а смешиваем арт с золотым градиентом
    // Чем ярче исходный пиксель (lum), тем больше в нем "металла"
    vec3 metalBase = mix(goldDark, goldBright, lum);
    
    // Добавляем эффект "жидкого" перелива
    metalBase += goldBright * streak2 * 0.5;
    
    // 5. Финальный композит
    // Смешиваем оригинальный арт с золотом через sparkLife
    // При sparkLife = 1.0 перс полностью золотой, при 0.5 - золотой отблеск
    vec3 finalGold = mix(color_out.rgb, metalBase, sparkLife);
    
    // Добавляем самый яркий "алмазный" блик поверх всего
    finalGold += goldWhite * streak1 * sparkLife * 1.5;
    
    // Привязываем блеск к альфе, чтобы не светились пустые места
    color_out.rgb = finalGold * color_out.a;
    
    // 6. Опционально: эффект "сияния богатства"
    // Легкий HDR-пересвет на самых ярких пикселях
    color_out.rgb += goldBright * pow(streak1, 3.0) * sparkLife * 2.0;
}
else if (effectType > 15.5 && effectType < 16.5) { // Cosmic Singularity 2026
    // 1. Создаем локальные координаты относительно "центра" детали
    // Так как мы не знаем центр в атласе, мы эмулируем его через шум от UV
    vec2 localUV = uv_main - 0.5; // Условный центр
    float dist = length(localUV);
    
    // 2. Эффект "Тоннеля" (Inversion)
    // Мы заставляем текстуру бесконечно "спадать" внутрь
    float tunnel = fract(1.0 / (dist + 0.05) + u_time * 1.5);
    
    // 3. Динамический паттерн Бездны
    // Создаем "энергетические кольца", которые уходят вглубь
    float ring = smoothstep(0.4, 0.5, sin(tunnel * 6.28));
    
    // 4. Цветовая схема "Хроматическая Тьма"
    // Глубокий синий, переходящий в неоновый фиолетовый на краях "колец"
    vec3 abyssColor = mix(vec3(0.02, 0.0, 0.1), vec3(0.4, 0.0, 0.8), ring);
    
    // 5. РЕАКЦИЯ НА АРТ (Самое мясо)
    // Мы берем оригинальный цвет (color_out) и используем его как карту высот
    // Кольца бездны будут "обтекать" светлые участки (лицо, доспехи)
    float mask = pow(1.0 - lum, 2.0) * sparkLife;
    
    // 6. Хроматическая аберрация света
    // Создаем радужный контур там, где бездна соприкасается с реальностью
    vec3 rainbow = vec3(
        sin(u_time * 5.0), 
        sin(u_time * 5.0 + 2.0), 
        sin(u_time * 5.0 + 4.0)
    ) * 0.5 + 0.5;
    
    // Собираем финальный пиксель
    // Оставляем светлые детали (доспехи), а тени превращаем в бесконечный тоннель
    color_out.rgb = mix(color_out.rgb, abyssColor * 2.0, mask);
    
    // Добавляем "искры реальности" на краях
    float edgeLight = smoothstep(0.1, 0.0, abs(dist - 0.25)) * sparkLife;
    color_out.rgb += rainbow * edgeLight * 1.5;
    
    // 7. Пульсация горизонта событий
    float pulse = sin(u_time * 10.0) * 0.1 + 0.9;
    color_out.rgb *= pulse;
}
else if (effectType > 16.5 && effectType < 17.5) { // Overlord Reality Glitch
    // 1. ЛОКАЛЬНОЕ ИСКАЖЕНИЕ (Скручивание без ухода за границы атласа)
    // Мы не двигаем UV, а используем синусоидальное "дыхание" внутри пикселя
    float t = u_time * 3.0;
    vec2 p = uv_main - 0.5; // Локальный центр
    float r = length(p);
    float angle = atan(p.y, p.x);
    
    // Эффект гравитационной волны: пиксели вибрируют к центру
    float wave = sin(r * 15.0 - t) * 0.05 * sparkLife;
    
    // 2. СПЕКТРАЛЬНАЯ ПЛАЗМА (Чистое HDR-мясо)
    // Генерируем три слоя энергии, которые "вращаются" внутри
    float energy1 = sin(angle * 3.0 + t) * 0.5 + 0.5;
    float energy2 = sin(angle * 5.0 - t * 0.7) * 0.5 + 0.5;
    float energy3 = sin(r * 10.0 + t * 1.5) * 0.5 + 0.5;
    
    // Цвета: Ультрафиолет, Огненное Золото и Глубокий Циан
    vec3 col1 = vec3(0.5, 0.0, 1.0); // Violet
    vec3 col2 = vec3(1.0, 0.5, 0.0); // Gold
    vec3 col3 = vec3(0.0, 1.0, 1.0); // Cyan
    
    vec3 plasma = col1 * energy1 + col2 * energy2 + col3 * energy3;
    plasma *= (1.0 - r); // Гасим к краям
    
    // 3. ЭФФЕКТ "ЧЕРНОГО СОЛНЦА" (Negative Core)
    // В центре персонажа образуется инвертированное ядро
    float coreMask = smoothstep(0.3 * sparkLife, 0.0, r);
    vec3 invertedArt = 1.0 - color_out.rgb;
    
    // 4. КОМПОЗИТ (Сборка челюстедробилки)
    // Мы смешиваем оригинальный арт с плазмой
    color_out.rgb = mix(color_out.rgb, plasma * 3.0, sparkLife * (1.0 - lum));
    
    // Накладываем инвертированное ядро в центре
    color_out.rgb = mix(color_out.rgb, invertedArt + col1, coreMask * sparkLife);
    
    // 5. РАДУЖНАЯ АБЕРРАЦИЯ КОНТУРА (Spectral Rim)
    // Подсвечиваем края каждой детали Spine радужным "бензином"
    float rim = pow(1.0 - color_out.a, 2.0) * color_out.a; // Мягкий внутренний край
    vec3 rimCol = vec3(sin(t), sin(t + 2.0), sin(t + 4.0)) * 0.5 + 0.5;
    color_out.rgb += rimCol * rim * 5.0 * sparkLife;
    
    // 6. ФИНАЛЬНЫЙ ШТРИХ: Световой удар
    // Каждые пару секунд босс ослепительно вспыхивает (как молния 4.1)
    float burst = pow(max(0.0, sin(u_time * 2.0)), 20.0);
    color_out.rgb += vec3(1.0) * burst * sparkLife * 2.0;
    
    // Применяем искажение волны к яркости (эффект дрожания пространства)
    color_out.rgb *= (1.0 + wave * 10.0 * sparkLife);
}
else if (effectType > 17.5 && effectType < 18.5) { // Chronos Overlord
    // 1. ТАЙМИНГИ (Средняя скорость — чтобы глаз успевал за цветом)
    float t = u_time * 2.0;
    float snapTime = floor(u_time * 10.0);
    
    // 2. КРИСТАЛЛИЧЕСКАЯ КАУСТИКА
    vec2 p = uv_main - 0.5;
    float angle = atan(p.y, p.x);
    float r = length(p);
    
    // Грани кристалла (facets) — делаем их четкими
    float facets = sin(uv_main.x * 12.0 + snapTime) * cos(uv_main.y * 12.0 - snapTime);
    float crystal = smoothstep(0.35, 0.45, facets * (1.1 - r));
    
    // 3. СОЧНАЯ РАДУГА (Возвращаем яркость, но убираем кислоту)
    // Используем косинусы для более плавных цветовых гармоник
    // Добавляем r в расчет, чтобы радуга "обволакивала" меш
    vec3 rainbow = 0.5 + 0.5 * cos(t + angle + vec3(0, 2.09, 4.18) + r * 3.0);
    
    // 4. ЖИДКОЕ ЗОЛОТО (Silk Gold)
    float streak = pow(max(0.0, sin(angle * 5.0 - t * 1.5)), 10.0);
    vec3 goldBase = mix(vec3(0.3, 0.15, 0.0), vec3(1.0, 0.8, 0.3), lum);
    
    // 5. КОМПОЗИТ (Баланс между золотом и радугой)
    // Оставляем золото в тенях, а радугу пускаем по граням
    vec3 baseArt = mix(color_out.rgb, goldBase, sparkLife * 0.6);
    
    // ВНИМАНИЕ: Накладываем радугу через сложение (ADD), чтобы она СИЯЛА
    // Это вернет те самые переливы, от которых захватывает дух
    vec3 rainbowLayer = rainbow * crystal * 1.8 * sparkLife;
    baseArt += rainbowLayer;
    
    // Добавляем золотые "иглы" света (кремовый блик)
    baseArt += vec3(1.0, 0.9, 0.7) * streak * sparkLife * 1.5;
    
    // 6. МАГИЧЕСКИЙ КОНТУР (Rim Distortion)
    float edge = pow(1.0 - color_out.a, 2.0) * color_out.a;
    float edgeWave = sin(angle * 20.0 + t * 10.0) * 0.5 + 0.5;
    // Контур теперь тоже радужный и яркий!
    baseArt += rainbow * edge * edgeWave * 10.0 * sparkLife;
    
    // 7. ИМПУЛЬС (Короткая радужная вспышка)
    float bang = pow(max(0.0, sin(u_time * 2.0)), 40.0);
    baseArt += rainbow * bang * 3.0 * sparkLife;
    
    // 8. ИТОГ
    color_out.rgb = baseArt;
    
    // Легкое мерцание яркости для "живого" эффекта
    color_out.rgb *= (0.95 + 0.1 * sin(t * 3.0));
    color_out.a *= (0.95 + 0.05 * cos(t));
}
else if (effectType > 18.5 && effectType < 19.5) { // The End of All Things
    // 1. ВРЕМЯ И ПУЛЬСАЦИЯ
    float t = u_time * 1.2;
    
    // 2. ТЕМНАЯ БАЗА (Затемняем персонажа в фиолет)
    // Уходим от белого в глубокий "космический" пурпур
    vec3 deepPurple = vec3(0.05, 0.0, 0.12);
    vec3 midViolet = vec3(0.3, 0.0, 0.6);
    
    // 3. АНИЗОТРОПНЫЕ ГРАНИ (Electric Edges)
    // Делаем линии чуть тоньше и "злее"
    float d1 = sin(uv_main.x * 15.0 + uv_main.y * 10.0 + t);
    float d2 = sin(uv_main.x * -10.0 + uv_main.y * 15.0 - t * 0.8);
    float facets = max(pow(max(0.0, d1), 18.0), pow(max(0.0, d2), 18.0));
    
    // 4. СПЕКТРАЛЬНЫЙ ПЕРЕЛИВ (Night Rainbow)
    // Ограничиваем радугу только холодными оттенками (синий, фиолет, розовый)
    float rainbowMap = lum * 4.0 + t;
    vec3 nightRainbow = vec3(
        sin(rainbowMap) * 0.5 + 0.5,      // Red (для розового)
        sin(rainbowMap + 1.5) * 0.2,      // Green (минимум зеленого!)
        sin(rainbowMap + 3.0)             // Blue (основной)
    );
    
    // 5. "ГОРЯЩИЕ" ЖИЛЫ (Amethyst Fire)
    // Самые светлые участки арта превращаются в раскаленный неон
    float hotSpot = pow(lum, 4.0) * sparkLife;
    
    // 6. КОМПОЗИТ (Мрачное Величие)
    // Сначала тонируем персонажа в темный аметист
    color_out.rgb = mix(color_out.rgb, deepPurple, sparkLife * 0.8);
    
    // Подсвечиваем детали через мистический фиолетовый
    color_out.rgb = mix(color_out.rgb, midViolet + nightRainbow * 0.5, sparkLife * 0.5 * (1.0 - hotSpot));
    
    // Добавляем электрические грани (неоновый синий/фиолет)
    vec3 electricCol = vec3(0.5, 0.2, 1.0);
    color_out.rgb += electricCol * facets * sparkLife * 3.0;
    
    // Вспышки "ядра" (hotspots) теперь тоже фиолетово-синие
    color_out.rgb += nightRainbow * hotSpot * 8.0 * sparkLife;
    
    // 7. СИЯНИЕ БЕЗДНЫ (Rim Aura)
    // Края светятся глубоким индиго
    float rim = pow(1.0 - lum, 3.0) * color_out.a * sparkLife;
    color_out.rgb += vec3(0.4, 0.1, 0.8) * rim * 5.0;
    
    // 8. ЭФФЕКТ "ЧЕРНОГО ЗЕРКАЛА"
    // Добавляем легкий контраст, чтобы темные места "проваливались"
    color_out.rgb -= (1.0 - sparkLife) * 0.05;
    color_out.rgb *= (0.9 + 0.2 * sin(u_time * 1.5) * sparkLife);
}
else if (effectType > 19.5 && effectType < 20.5) { // GENESIS: THE BIG BANG
    // 1. КИНЕТИКА (Время течет по-разному в разных частях тела)
    float t = u_time * 3.0;
    float chronoStep = floor(u_time * 24.0); // Кинематографичный "джиттер"
    
    // 2. ГИПЕР-ПРОСТРАНСТВО (The Fold)
    vec2 p = uv_main - 0.5;
    float r = length(p);
    float angle = atan(p.y, p.x);
    
    // Эффект "линзы": искажаем центр, но оставляем края атласа в покое
    float lens = pow(r, 2.0) * sparkLife;
    float zoom = 1.0 - sparkLife * 0.5;
    
    // 3. КВАНТОВЫЙ ШУМ (БЕЗ текстур, только математика)
    // Генерируем "живую плазму" через интерференцию 4-х волн
    float noise = sin(p.x * 20.0 + t) + sin(p.y * 15.0 - t * 0.8);
    noise += sin((p.x + p.y) * 10.0 + t * 1.2);
    noise = abs(fract(noise) - 0.5) * 2.0; // "Рваные" края энергии
    
    // 4. ЦВЕТОВАЯ ДЕСТРУКЦИЯ (Chromatic Core)
    // Создаем эффект "бензинового Бога"
    vec3 spectral = 0.5 + 0.5 * cos(chronoStep + angle + vec3(0.0, 2.1, 4.2));
    
    // 5. КОМПОЗИТ (Абсолютное Мясо)
    // Оригинальный арт превращаем в "золотой пепел"
    vec3 art = color_out.rgb;
    vec3 goldAsh = mix(art, vec3(1.0, 0.7, 0.2) * lum, sparkLife);
    
    // Внедряем спектральный разлом в структуру арта
    // Чем выше яркость (lum), тем сильнее "пробивает" свет
    vec3 core = mix(goldAsh, spectral * 3.0, noise * sparkLife * (1.0 - lum));
    
    // 6. СУПЕР-КОНТУР (Supernova Outline)
    // Края светятся так сильно, что "съедают" персонажа
    float rim = pow(1.0 - color_out.a, 3.0) * color_out.a;
    float wave = sin(angle * 40.0 + t * 10.0) * 0.5 + 0.5;
    core += spectral * rim * 15.0 * sparkLife * wave;
    
    // 7. ФИНАЛЬНЫЙ "ПШИК" (The Glitch Stroke)
    // Раз в пару секунд весь персонаж инвертируется по яркости
    float strobe = step(0.95, sin(u_time * 5.0)) * sparkLife;
    color_out.rgb = mix(core, 1.5 - core, strobe);
    
    // 8. ПРОСТРАНСТВЕННЫЙ СДВИГ (RGB Split)
    // Сдвигаем цвета без захода на соседние спрайты
    color_out.r = mix(color_out.r, color_out.g, sparkLife * 0.3 * strobe);
    color_out.b = mix(color_out.b, spectral.b, sparkLife * 0.5 * strobe);

    // Персонаж начинает "вибрировать" и исчезать в радиации
    color_out.a *= (1.0 - pow(sparkLife, 5.0) * 0.5);
    color_out.rgb *= (1.0 + noise * sparkLife * 0.5);
}
else if (effectType > 20.5 && effectType < 21.5) { // Mercury Dissolve
    // 1. ПАРАМЕТРЫ ГРАВИТАЦИИ
    // Мы заставляем "шум" течь вниз, имитируя падение капель
    float dropSpeed = u_time * 2.5;
    vec2 dropUV = uv_main * vec2(8.0, 4.0); // Масштабируем: вытянутые капли
    
    // 2. ГЕНЕРАЦИЯ КАПЕЛЬ (SDF-подобный шум)
    // Используем sin от смещенных координат, чтобы создать "пузыри"
    float droplets = sin(dropUV.x + sin(dropUV.y + dropSpeed)) * cos(dropUV.y - dropSpeed);
    droplets = smoothstep(0.0, 0.1, droplets - (1.0 - sparkLife)); // Капли "съедают" массу
    
    // 3. ЦВЕТ: ЖИДКИЙ МЕТАЛЛ (Ртуть)
    vec3 mercury = vec3(0.8, 0.8, 0.9);
    // Блик на каждой капле (fake shading)
    float spec = pow(max(0.0, droplets), 10.0); 
    
    // 4. ДЕФОРМАЦИЯ СИЛУЭТА
    // Мы используем нашу маску капель, чтобы "продырявить" персонажа
    // color_out.a становится рваным
    float finalAlpha = color_out.a * droplets;
    
    // 5. ИСКАЖЕНИЕ ФОРМЫ (Мясо)
    // Чтобы казалось, что капли РЕАЛЬНО текут, мы слегка смещаем цвет
    // Но делаем это ОЧЕНЬ аккуратно (всего на 0.005), чтобы не вылезти за атлас
    float uvShift = (1.0 - droplets) * 0.005 * sparkLife;
    
    // 6. КОМПОЗИТ
    // Смешиваем оригинальный арт с металлом
    color_out.rgb = mix(color_out.rgb, mercury + spec * 2.0, sparkLife);
    
    // Добавляем эффект "стекания" по краям
    float edgeGlow = smoothstep(0.1, 0.0, finalAlpha) * color_out.a;
    color_out.rgb += vec3(1.0) * edgeGlow * sparkLife;

    // Применяем разрушенную альфу
    color_out.a = finalAlpha;
    
    // 7. ФИНАЛЬНЫЙ ШТРИХ: Растяжение вниз
    // В конце жизни эффекта капли "вытягиваются"
    if (sparkLife < 0.5) {
        color_out.a *= smoothstep(0.0, 0.5, uv_main.y + sparkLife);
    }
}
else if (effectType > 21.5 && effectType < 22.5) { // Liquid Inferno (Magma Flow)
    // 1. УПРАВЛЕНИЕ (оставляем твою рабочую рябь)
    float flow = sparkLife * 4.0;
    vec2 uv = uv_main * vec2(40.0, 6.0);
    
    float n1 = sin(uv.x + flow);
    float n2 = sin(uv.x * 0.6 - flow * 1.2 + uv.y);
    float n3 = sin(uv.y * 3.0 + flow * 2.0);
    float ripple = abs(n1 + n2 + n3) / 3.0;
    
    // 2. ЦВЕТОВАЯ ПАЛИТРА (Фиолетовое Инферно)
    // Базовый цвет: Глубокий фиолетовый
    vec3 magicBase = vec3(0.6, 0.1, 1.0); 
    // Цвет блика: Неоновый розово-белый
    vec3 magicShine = vec3(1.0, 0.4, 1.0); 
    
    // 3. КОМПОЗИТ
    // Острый блик
    float spec = pow(ripple, 15.0); 
    
    // Смешиваем фиолетовую базу с яркими розовыми вспышками
    // Добавляем spec * 3.0 для экстремального сияния на гребнях
    color_out.rgb = (magicBase + magicShine * spec * 3.0) * color_out.a;
    
    // Пульсация альфы для "магического" объема
    color_out.a *= (0.85 + ripple * 0.15);
}
else if (effectType > 22.5 && effectType < 23.5) { // Liquid Inferno (Magma Flow)
    // 1. Создаем турбулентность (вместо карты высот золота)
    // Используем uv и время, чтобы создать эффект "кипения"
    float noise = sin(uv_main.x * 7.0 + u_time * 2.0) * cos(uv_main.y * 8.0 - u_time * 1.5);
    float flow = lum + noise * 0.3;
    
    // 2. Двойная волна жара (вместо узких бликов металла)
    // Плазма движется хаотично: одна волна вверх, другая вбок
    float heat1 = sin(uv_main.y * 12.0 - u_time * 4.0 + flow * 3.0);
    float heat2 = sin((uv_main.x + uv_main.y) * 6.0 + u_time * 2.0);
    
    // Делаем "языки" пламени мягче, чем золото, но с горячим центром
    float pulse1 = pow(max(0.0, heat1), 4.0); // Мягкие сгустки
    float pulse2 = pow(max(0.0, heat2), 6.0);
    
    // 3. Цветовая палитра "Deep Inferno"
    vec3 fireCore = vec3(1.0, 0.9, 0.4);   // Почти белый (центр жара)
    vec3 fireOrange = vec3(1.0, 0.4, 0.0); // Основной огонь
    vec3 fireRed = vec3(0.6, 0.05, 0.0);   // Остывающая магма
    
    // 4. Логика "внутреннего свечения"
    // Смешиваем цвета в зависимости от яркости (lum) и турбулентности
    vec3 plasmaBase = mix(fireRed, fireOrange, flow);
    
    // Добавляем "вспышки" жара внутри облака
    plasmaBase += fireOrange * pulse2 * 0.8;
    
    // 5. Финальный композит
    // Смешиваем арт (дым) с этой плазмой через sparkLife
    vec3 finalFire = mix(color_out.rgb, plasmaBase, sparkLife);
    
    // Добавляем самый горячий "белый" жар в центры пульсаций
    finalFire += fireCore * pulse1 * sparkLife * 2.0;
    
    // Привязываем к альфе
    color_out.rgb = finalFire * color_out.a;
    
    // 6. Эффект Иррадиации (свечение вокруг)
    // Заставляем частицу "выжигать" экран
    color_out.rgb += fireOrange * pow(pulse1, 2.0) * sparkLife * 1.5;
}
else if (effectType > 23.5 && effectType < 24.5) { // Energy Manifestation (Appearance)
    // progress: 0.0 - пустота, 1.0 - персонаж полностью проявлен
    float progress = sparkLife; 
    
    // 1. Искажение координат (эффект "затягивания" в портал)
    // Чем меньше progress, тем сильнее дрожит текстура
    float distAmt = (1.0 - progress) * 0.05;
    vec2 distortedUV = uv_main + vec2(
        sin(u_time * 10.0 + uv_main.y * 20.0) * distAmt,
        cos(u_time * 12.0 + uv_main.x * 20.0) * distAmt
    );
    
    // 2. Маска появления (шумный срез)
    // Генерируем "рваный" край, чтобы не было скучной прямой линии
    float noise = fract(sin(dot(uv_main.xy, vec2(12.9898, 78.233))) * 43758.5453);
    // Смешиваем координату Y с шумом для эффекта магической эрозии
    float revealMask = (1.0 - uv_main.y) + (noise - 0.5) * 0.15;
    
    // Плавный порог проявления
    float threshold = progress * 1.2 - 0.1;
    float visibility = smoothstep(threshold - 0.05, threshold + 0.05, revealMask);
    
    // 3. Магическая кромка (то самое "УУУУ")
    // Создаем яркую полосу на самом краю маски visibility
    float edge = smoothstep(0.4, 0.5, 1.0 - abs(visibility - 0.5) * 2.0);
    edge = pow(edge, 3.0); // Делаем кромку поострее
    
    // 4. Цвета магии
    vec3 magicColor1 = vec3(0.7, 0.0, 1.0); // Ярко-фиолетовый
    vec3 magicColor2 = vec3(0.0, 0.8, 1.0); // Электрический неон
    // Переливающийся цвет для кромки
    vec3 edgeGlow = mix(magicColor1, magicColor2, sin(u_time * 5.0) * 0.5 + 0.5);
    
    // 5. Финальный сбор
    // Берем оригинальный цвет персонажа из текстуры
    vec3 finalRGB = color_out.rgb;
    
    // Добавляем свечение на край появления
    // Оно накладывается ПОВЕРХ родных цветов
    finalRGB += edgeGlow * edge * 3.0;
    
    // 6. Прозрачность
    // Персонаж полностью прозрачен там, где маска еще не прошла
    float finalAlpha = color_out.a * visibility;
    
    // Эффект "исчезающих искр" выше линии появления
    if (visibility < 0.1) {
        float sparkles = pow(noise, 50.0) * (1.0 - progress) * 2.0;
        finalAlpha = max(finalAlpha, sparkles * color_out.a);
        finalRGB = edgeGlow * 2.0;
    }
    
    color_out.rgb = finalRGB * finalAlpha;
    color_out.a = finalAlpha;
}
else if (effectType > 24.5 && effectType < 25.5) { // Void Singularity (Kali Manifest)
    float progress = sparkLife; 
    
    // 1. СОЗДАЕМ ИДЕАЛЬНЫЕ ЭКРАННЫЕ КООРДИНАТЫ (0.0 до 1.0)
    // u_resolution делает их независимыми от размера окна
    vec2 screenUV = gl_FragCoord.xy / u_resolution.xy;
    
    // Центрируем (0.5, 0.5 — теперь всегда центр экрана)
    vec2 centeredUV = screenUV - vec2(0.5, 0.5);
    
    // Исправляем растягивание (чтобы круг не был овалом на широкоформатных мониторах)
    centeredUV.x *= u_resolution.x / u_resolution.y;
    
    // 2. ИСКАЖЕНИЕ "ЧЕРНАЯ ДЫРА"
    float dist = length(centeredUV);
    float pull = pow(1.0 - progress, 3.0) * 0.2;
    
    // Стягиваем координаты к центру
    vec2 distortedUV = centeredUV - (centeredUV * pull * sin(dist * 10.0 - u_time * 5.0));
    
    // 3. ФРАКТАЛЬНЫЙ РАЗЛОМ (Бесшовный)
    float crack = sin(distortedUV.x * 20.0 + u_time * 3.0) * cos(distortedUV.y * 15.0 - u_time * 2.0);
    
    // Маска появления (растет из центра)
    float mask = smoothstep(progress - 0.2, progress + 0.1, (1.0 - dist) + crack * 0.2);
    
    // 4. ТВОИ ЛЮБИМЫЕ ЦВЕТА КАЛИ
    vec3 voidDeep = vec3(0.1, 0.0, 0.2);   
    vec3 kaliNeon = vec3(0.6, 0.0, 1.0);   
    vec3 electricCyan = vec3(0.0, 1.0, 1.0); 
    
    // 5. ЭФФЕКТ КРОМКИ И МОЛНИИ
    float edge = smoothstep(0.4, 0.5, 1.0 - abs(mask - 0.5) * 2.0);
    float lightning = pow(abs(crack), 10.0) * edge; 
    
    // 6. СБОРКА ЦВЕТА (Твой оригинальный алгоритм)
    vec3 baseColor = color_out.rgb;
    vec3 magicEffect = mix(voidDeep, kaliNeon, crack * 0.5 + 0.5);
    
    vec3 finalRGB = mix(magicEffect, baseColor, mask);
    
    // Добавляем неон и молнии
    finalRGB += kaliNeon * edge * 2.0;
    finalRGB += electricCyan * lightning * 3.0;
    
    // ИНВЕРСИЯ (Крышеснос)
    float flash = pow(edge, 5.0) * (1.0 - progress);
    finalRGB = mix(finalRGB, vec3(1.0) - finalRGB, flash);
    
    // 7. ПРОЗРАЧНОСТЬ (Строго по альфе аттачмента!)
    float finalAlpha = color_out.a * mask;
    
    // Аура вокруг
    if (finalAlpha < 0.2) {
        finalAlpha = edge * 0.4 * color_out.a;
        finalRGB = kaliNeon;
    }
    
    color_out.rgb = finalRGB * finalAlpha;
    color_out.a = finalAlpha;

}
else if (effectType > 25.5 && effectType < 26.5) { // Thor's Storm (God of Thunder)
    float progress = sparkLife;
    
    // 1. Уходим от линий к "Пятнам" (Plasma Noise)
    // Используем синусы от X и Y вместе, чтобы запутать направление
    float speed = u_time * 0.4;
    float noise = sin(uv_main.x * 3.0 + speed) + 
                  sin(uv_main.y * 2.0 - speed * 1.2) + 
                  sin((uv_main.x + uv_main.y) * 1.5 + speed);
    
    // 2. Генерируем "Круговой" спектр
    // Цвета теперь зависят не от наклона, а от хаотичного шума
    vec3 rainbow;
    rainbow.r = sin(noise + 0.0) * 0.5 + 0.5;
    rainbow.g = sin(noise + 2.0) * 0.5 + 0.5;
    rainbow.b = sin(noise + 4.0) * 0.5 + 0.5;
    
    // 3. Мягкое наложение (Screen / Color Dodge)
    // Чтобы фон не превращался в кашу, подмешиваем радугу аккуратно
    vec3 baseColor = color_out.rgb;
    
    // Создаем эффект "Северного сияния"
    // Радуга будет ярче там, где шум сильнее, и мягче в остальных местах
    vec3 aurora = baseColor + (rainbow * 0.6);
    
    // 4. Добавляем "Глубину" через яркость
    // Пусть радуга "затекает" в основном в светлые и средние тона
    vec3 finalRGB = mix(baseColor, aurora, progress * (lum * 0.5 + 0.5));
    
    // 5. Магический "Глянец"
    // Добавим белое мерцание, которое плавает как блики на воде
    float glint = pow(max(0.0, sin(noise * 2.0 + u_time)), 20.0);
    finalRGB += vec3(0.8, 0.9, 1.0) * glint * progress * 0.5;
    
    // 6. Финальный вывод
    color_out.rgb = finalRGB * color_out.a;
    
    // 7. Эффект HDR
    // Самые сочные цвета радуги будут немного светиться
    color_out.rgb += rainbow * pow(glint, 2.0) * progress;
}

    vec2 p = gl_FragCoord.xy / u_resolution;

    // Соотношение сторон для честного круга
    float aspect = u_resolution.x / u_resolution.y;
    vec2 p_asp = p * vec2(aspect, 1.0);

    float glow = 0.0;
    
    // Считаем маски (используем 4-ю степень для мягкости: m*m*m*m)
    // Умножаем центры u_hit на аспект прямо здесь
    
    if (u_hitI0 > 0.001) {
        float m = smoothstep(u_hit0.z, 0.0, distance(p_asp, u_hit0.xy * vec2(aspect, 1.0)));
        glow += (m * m * m * m) * u_hitI0;
    }
    if (u_hitI1 > 0.001) {
        float m = smoothstep(u_hit1.z, 0.0, distance(p_asp, u_hit1.xy * vec2(aspect, 1.0)));
        glow += (m * m * m * m) * u_hitI1;
    }
    if (u_hitI2 > 0.001) {
        float m = smoothstep(u_hit2.z, 0.0, distance(p_asp, u_hit2.xy * vec2(aspect, 1.0)));
        glow += (m * m * m * m) * u_hitI2;
    }
    if (u_hitI3 > 0.001) {
        float m = smoothstep(u_hit3.z, 0.0, distance(p_asp, u_hit3.xy * vec2(aspect, 1.0)));
        glow += (m * m * m * m) * u_hitI3;
    }
    if (u_hitI4 > 0.001) {
        float m = smoothstep(u_hit4.z, 0.0, distance(p_asp, u_hit4.xy * vec2(aspect, 1.0)));
        glow += (m * m * m * m) * u_hitI4;
    }

    // Итоговый цвет света с небольшим приглушением (0.6)
    vec3 final_glow = u_hitColor * glow * 1.5;

    // Режим Screen: мягко накладывает свет, не превращая всё в белое пятно
    color_out.rgb = 1.0 - (1.0 - color_out.rgb) * (1.0 - final_glow);
//    color_out.rgb += u_hitColor * glow;


    color_out *= tintColor;
    color_out.rgb += addRGB.rgb;

// --- 4. ПОСТ-ЭФФЕКТЫ ---
    color_out.rgb *= color_out.a;
    color_out.a *= alphaBlend;

    if (u_lowHPEffect > 0.0) {
        float hp_dist = distance(uv_main, vec2(0.5));
        float pulse = smoothstep(0.3, 0.8, hp_dist) * ((sin(u_time * 5.0) * 0.5 + 0.5) * u_lowHPEffect) * color_out.a;
        color_out.rgb = mix(color_out.rgb, vec3(0.8, 0.0, 0.0) * color_out.a, pulse * 0.7);
    }

    // --- 5. ТУТОРИАЛ (SDF логика выполняется только при наличии v_tutIntensity) ---
    if (tutIntensity > 0.0) {
        vec2 aspect_vec = vec2(u_resolution.x / (u_resolution.y + 0.001), 1.0);
        vec2 p_tut = (gl_FragCoord.xy / u_resolution - u_tutParams.xy) * aspect_vec;
        vec2 d_tut = abs(p_tut) - (u_tutRectSize * aspect_vec) + u_tutParams.z;
        float tut_dist = length(max(d_tut, 0.0)) + min(max(d_tut.x, d_tut.y), 0.0) - u_tutParams.z;
        
        float mask = smoothstep(0.0, 0.005, tut_dist);
        color_out.rgb *= mix(1.0, 1.0 - tutIntensity, mask);
        
        // Эффект мерцания рамки
        float pulse_tut = sin(u_time * 4.0) * 0.1 + 1.0;
        color_out.rgb *= mix(pulse_tut, 1.0, mask);
        
        // Бегущая полоса по рамке
        float edge_glow = smoothstep(0.004, 0.0, abs(tut_dist)) * smoothstep(0.4, 0.5, sin(p_tut.y * 15.0 - u_time * 8.0)) * 0.5 * tutIntensity;
        color_out.rgb += edge_glow;
    }

    gl_FragColor = color_out;
}