import fs from 'node:fs';
import {defineConfig} from 'astro/config';
import starlight from '@astrojs/starlight';

let pythonApiSidebar = [];
if (fs.existsSync(new URL('./src/generated/python-api-sidebar.ts', import.meta.url))) {
    ({pythonApiSidebar} = await import('./src/generated/python-api-sidebar.ts'));
}
let cppApiSidebar = [];
if (fs.existsSync(new URL('./src/generated/cpp-api-sidebar.ts', import.meta.url))) {
    ({cppApiSidebar} = await import('./src/generated/cpp-api-sidebar.ts'));
}

export default defineConfig({
    integrations : [
        starlight({
            title : 'FreeCAD API',
            sidebar : [ {link : '/', label : 'Overview'}, ...pythonApiSidebar, ...cppApiSidebar ],
        }),
    ],
});
