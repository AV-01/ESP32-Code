let settings = null;
let loadedFonts = [];
let hershey_simplex;
let initGCode = "";
let endGCode = "";
async function loadSettings(){
    try{
        const response1 = await fetch('/fonts/default_settings.json');
        if(response1.ok){
            settings = await response1.json();
        }
    }
    catch(e){
        console.error("Failed to load settings", e);
    }
}

async function loadSimplex(){
    try{
        const response1 = await fetch('/fonts/hershey_simplex.json');
        if(response1.ok){
            hershey_simplex = await response1.json();
        }
    }
    catch(e){
        console.error("Failed to load simplex font", e);
    }
}

async function loadFont(fileName){
    try{
        const response1 = await fetch(fileName);
        if(response1.ok){
            loadedFonts.push(await response1.json());
        }
    }
    catch(e){
        console.error(`Failed to fetch ${fileName}`, e);
    }
}

async function loadGCodeDefaults(){
    try{
        const response1 = await fetch('/initialize_gcode.txt');
        if(response1.ok){
            const text = await response1.text();
            initGCode = text.trim().split('\n').map(line => line.split(";")[0].trim()).filter(line => line.length > 0).join("\n") + '\n';
        }

        const response2 = await fetch('/end_gcode.txt');
        if(response2.ok){
            const text = await response2.text();
            endGCode = text.trim().split('\n').map(line => line.split(";")[0].trim()).filter(line => line.length > 0).join("\n") + '\n';
        }
    }
    catch(e){
        console.error("Failed to load GCode Defaults", e);
    }
}

loadSettings();
loadSimplex();
loadGCodeDefaults();
loadFont("/fonts/font1.json");
loadFont("/fonts/font2.json");

function getTextStrokes(text, fontSize){
    if(!settings || !text) return [];
    const fontDicts = loadedFonts.length > 0 ? loadedFonts : (hershey_simplex ? [hershey_simplex] : []);
    if(fontDicts.length === 0) return [];

    const scale = fontSize/21.0;
    const lineHeight = fontSize * 1.5;

    let strokes = [];
    const max_width = (settings.X_MAX - settings.X_MIN - settings.LEFT_MARGIN );
    let cursor_x = settings.LEFT_MARGIN;
    let cursor_y = -(settings.TOP_MARGIN + (settings.FLOAT_OFFSET || 0));

    const lines = text.split('\n');

    for(const line of lines){
        const words = line.split(' ');
        for(const word of words){
            let word_width = 0;
            for(const char of word){
                let validFonts = fontDicts.filter(f => f[char]);
                if(validFonts.length > 0){
                    let data = validFonts[0][char];
                    if(data && data.length >= 2){
                        word_width += (data[1] - data[0]) * scale;
                    }
                }
            }

            if(cursor_x + word_width > settings.LEFT_MARGIN + max_width && cursor_x > settings.LEFT_MARGIN){
                cursor_x = settings.LEFT_MARGIN;
                cursor_y -= lineHeight;
            }

            for(const char of word){
                let validFonts = fontDicts.filter(f => f[char]);
                let data = null;
                if(validFonts.length > 0){
                    let activeFont = validFonts[Math.floor(Math.random() * validFonts.length)];
                    data = activeFont[char];
                }

                if(!data || data.length < 2) continue;

                const char_left = data[0];
                const char_right = data[1];
                const char_width = (char_right - char_left) * scale;
                const char_offset_x = cursor_x - (char_left * scale);

                for (let i = 2; i < data.length; i++){
                    const path = data[i];
                    if(!path || !Array.isArray(path)) continue;
                    let stroke = [];

                    for(const pt of path){
                        if(pt.length < 2) continue;
                        let mx = char_offset_x + (pt[0] * scale);
                        let my = cursor_y + (pt[1] * scale);
                        stroke.push([mx, my]);
                    }

                    if(stroke.length > 0) strokes.push(stroke);
                }
                cursor_x += char_width;
            }

            let space_font = fontDicts.find(f => f[' ']) || fontDicts[0];
            let space_data = space_font[' '] || [-10, 10];
            cursor_x += (space_data[1] - space_data[0]) * scale;
        }

        cursor_x = settings.LEFT_MARGIN;
        cursor_y -= lineHeight;
    }
    return strokes;
}

function transformPaperToMachine(px, py){
    return [settings.X_MIN + px, settings.Y_MAX + py];
}

async function generateGCode(text, fontSize){
    if(!settings) return "Error: Settings not loaded";
    let strokes = getTextStrokes(text, fontSize);
    let gcode = "";
    gcode += initGCode || "";
    gcode += `G0 Z${settings.Z_SAFE} F${settings.F_Z}\n`;

    for(const s of strokes){
        if(!s || s.length === 0) continue;
        let p0 = transformPaperToMachine(s[0][0], s[0][1]);
        if(p0[0] >= settings.X_MIN && p0[0] <= settings.X_MAX && p0[1] >= settings.Y_MIN && p0[1] <= settings.Y_MAX){
            gcode += `G0 X${p0[0].toFixed(2)} Y${p0[1].toFixed(2)} F${settings.F_TRAVEL}\n`;
            gcode += `G0 Z${settings.Z_DRAW} F${settings.F_Z}\n`;
            for(let i = 1; i < s.length; i++){
                let p = transformPaperToMachine(s[i][0], s[i][1]);
                gcode += `G1 X${p[0].toFixed(2)} Y${p[1].toFixed(2)} F${settings.F_DRAW}\n`;
            }
            gcode += `G0 Z${settings.Z_SAFE} F${settings.F_Z}\n`;
        }
    }
    gcode += endGCode || "";
    return gcode;
}