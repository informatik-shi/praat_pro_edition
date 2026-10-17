// Render and exercise the standalone HTML in a headless local browser.
// Set PLAYWRIGHT_MODULE if Playwright is outside the project's module search path.
const { chromium } = require(process.env.PLAYWRIGHT_MODULE || 'playwright');
const path = require('node:path');
const fs = require('node:fs/promises');
const { pathToFileURL } = require('node:url');

let browser;
(async () => {
  const root = path.resolve(__dirname, '..');
  const out = path.join(root, '.local-build/lua-textbook/preview');
  await fs.mkdir(out, {recursive:true});
  const browserOptions = {headless:true};
  if(process.env.PLAYWRIGHT_BROWSER_PATH) browserOptions.executablePath=process.env.PLAYWRIGHT_BROWSER_PATH;
  browser = await chromium.launch(browserOptions);
  const report = [];
  for (const lang of ['ru','en']) {
    const page = await browser.newPage({viewport:{width:1440,height:1000},deviceScaleFactor:1});
    const errors = [];
    page.on('pageerror', error => errors.push(error.message));
    const requests = [];
    page.on('request', request=>{if(/^https?:/.test(request.url())) requests.push(request.url())});
    await page.goto(pathToFileURL(path.join(root, `docs/tutorials/lua-praat/lua-praat-${lang}.html`)).href);
    await page.screenshot({path:path.join(out,`${lang}-cover.png`)});
    await page.locator('[id="16-formants"]').scrollIntoViewIfNeeded();
    await page.screenshot({path:path.join(out,`${lang}-lesson.png`)});
    await page.locator('[id="23-batch"]').scrollIntoViewIfNeeded();
    await page.screenshot({path:path.join(out,`${lang}-batch.png`)});
    const search = page.locator('#search');
    await search.fill(lang==='ru'?'форманты':'formants');
    const matches = await page.locator('nav a:visible').count();
    if (!matches || matches>=32) throw Error('Search is not filtering the contents');
    await search.fill('zzzzzzzzzz');
    if(await page.locator('nav a:visible').count()) throw Error('Empty search still shows matches');
    await search.fill('');
    const exercise = page.locator('[id="01-start"] details');
    await exercise.locator('summary').click();
    if(!(await exercise.getAttribute('open')=== '')) throw Error('Exercise answer did not open');
    const button = page.locator('[data-download="code-01-start"]');
    const downloaded = page.waitForEvent('download');
    await button.click();
    const download = await downloaded;
    const target = path.join(out,`${lang}-download.lua`);
    await download.saveAs(target);
    const expected = await fs.readFile(path.join(root,'docs/tutorials/lua-praat/examples/01-start.lua'),'utf8');
    if(await fs.readFile(target,'utf8')!==expected) throw Error('Downloaded Lua differs');
    await page.locator('[data-copy="code-01-start"]').click();
    await page.waitForFunction(()=>document.querySelector('[data-copy="code-01-start"]').textContent.includes(document.documentElement.lang==='ru'?'Скопировано':'Copied'));
    for (const width of [390,768,1440]) {
      await page.setViewportSize({width,height:900});
      await page.evaluate(()=>window.scrollTo(0,0));
      const overflow = await page.evaluate(()=>document.documentElement.scrollWidth>innerWidth);
      if(overflow) throw Error(`Page overflows at ${width}px`);
      if(width===390) await page.screenshot({path:path.join(out,`${lang}-mobile.png`)});
    }
    await page.emulateMedia({media:'print'});
    await page.pdf({path:path.join(out,`${lang}-print.pdf`),format:'A4',printBackground:true,preferCSSPageSize:true});
    if(errors.length||requests.length) throw Error(JSON.stringify({errors,requests}));
    report.push({lang,searchMatches:matches,download:true,copy:true,answers:true,viewports:[390,768,1440],offline:true,errors});
    await page.close();
  }
  await browser.close();
  await fs.writeFile(path.join(out,'browser-report.json'),JSON.stringify(report,null,2));
  console.log(JSON.stringify(report));
})().catch(async error=>{console.error(error);if(browser) await browser.close();process.exitCode=1});
