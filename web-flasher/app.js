(() => {
  const select = document.getElementById('test-select');
  const desc = document.getElementById('test-desc');
  const ack = document.getElementById('ack');
  const slot = document.getElementById('install-slot');
  const installStatus = document.getElementById('install-status');
  const buildInfo = document.getElementById('build-info');
  const monitorOutput = document.getElementById('monitor-output');
  const monitorStatus = document.getElementById('monitor-status');
  const connectButton = document.getElementById('monitor-connect');
  const disconnectButton = document.getElementById('monitor-disconnect');
  const clearButton = document.getElementById('monitor-clear');
  const saveButton = document.getElementById('monitor-save');

  let catalog = null;
  let serialPort = null;
  let serialReader = null;
  let stopReading = false;

  function selectedTest() {
    if (!catalog) return null;
    return catalog.tests.find((test) => test.id === select.value) || null;
  }

  function renderInstaller() {
    const test = selectedTest();
    desc.textContent = test ? test.description : '';
    slot.replaceChildren();

    if (!('serial' in navigator)) {
      installStatus.className = 'status warn';
      installStatus.textContent = 'Web Serial is not available in this browser. Use desktop Chrome or Edge over HTTPS.';
      return;
    }

    if (!ack.checked || !test) {
      installStatus.className = 'status';
      installStatus.textContent = 'Confirm the warning above to enable the installer.';
      return;
    }

    const installer = document.createElement('esp-web-install-button');
    installer.setAttribute('manifest', `manifests/${test.id}.json`);
    slot.appendChild(installer);
    installStatus.className = 'status ok';
    installStatus.textContent = `Ready to install: ${test.label}`;
  }

  async function loadCatalog() {
    try {
      const response = await fetch('tests.json', { cache: 'no-store' });
      if (!response.ok) throw new Error(`HTTP ${response.status}`);
      catalog = await response.json();
      select.replaceChildren();
      for (const test of catalog.tests) {
        const option = document.createElement('option');
        option.value = test.id;
        option.textContent = test.label;
        select.appendChild(option);
      }
      buildInfo.textContent = `${catalog.board.name} — ${catalog.board.flash} Flash / ${catalog.board.psram} PSRAM. Web images are produced by GitHub Actions from the repository Arduino BSP examples.`;
      renderInstaller();
    } catch (error) {
      buildInfo.textContent = `Could not load test catalog: ${error.message}`;
      buildInfo.className = 'status warn';
      installStatus.textContent = 'Installer unavailable until tests.json is published.';
      installStatus.className = 'status warn';
    }
  }

  function appendSerial(text) {
    monitorOutput.textContent += text;
    monitorOutput.scrollTop = monitorOutput.scrollHeight;
  }

  async function disconnectSerial() {
    stopReading = true;
    try {
      if (serialReader) {
        await serialReader.cancel();
        serialReader.releaseLock();
      }
    } catch (_) {}
    serialReader = null;

    try {
      if (serialPort) await serialPort.close();
    } catch (_) {}
    serialPort = null;

    connectButton.disabled = false;
    disconnectButton.disabled = true;
    monitorStatus.textContent = 'Disconnected.';
  }

  async function connectSerial() {
    if (!('serial' in navigator)) {
      monitorStatus.textContent = 'Web Serial is unavailable in this browser.';
      monitorStatus.className = 'status warn';
      return;
    }

    try {
      stopReading = false;
      serialPort = await navigator.serial.requestPort();
      await serialPort.open({ baudRate: 115200 });
      connectButton.disabled = true;
      disconnectButton.disabled = false;
      monitorStatus.className = 'status ok';
      monitorStatus.textContent = 'Connected at 115200 baud.';

      const decoder = new TextDecoder();
      serialReader = serialPort.readable.getReader();
      while (!stopReading) {
        const { value, done } = await serialReader.read();
        if (done) break;
        if (value) appendSerial(decoder.decode(value, { stream: true }));
      }
    } catch (error) {
      appendSerial(`\n[serial] ${error.message}\n`);
    } finally {
      if (serialPort || serialReader) await disconnectSerial();
    }
  }

  function saveLog() {
    const blob = new Blob([monitorOutput.textContent], { type: 'text/plain;charset=utf-8' });
    const url = URL.createObjectURL(blob);
    const anchor = document.createElement('a');
    anchor.href = url;
    anchor.download = `wt32-sc01-plus-${select.value || 'serial'}-${new Date().toISOString().replace(/[:.]/g, '-')}.log`;
    anchor.click();
    URL.revokeObjectURL(url);
  }

  select.addEventListener('change', renderInstaller);
  ack.addEventListener('change', renderInstaller);
  connectButton.addEventListener('click', connectSerial);
  disconnectButton.addEventListener('click', disconnectSerial);
  clearButton.addEventListener('click', () => { monitorOutput.textContent = ''; });
  saveButton.addEventListener('click', saveLog);

  loadCatalog();
})();
