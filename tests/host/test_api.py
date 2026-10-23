"""Exercise the actual C API/parser against malformed and boundary frames."""
import subprocess
import sys

frames = [
    '   \r\n', 'upx\x08time\r', 'led_period\r', 'led_period +1\r',
    'led_period -1\r', 'led_period 5x\r', 'led_period 4294967296\r',
    'led_period 1 2\r', 'led_period 0\r', 'led_period 4294967295\r',
    'led_disable ignored\r', 'led_period 1 2 3 4 5\r', 'led_blink\r',
    'x'*70+'\r', 'pi_start 1\r', 'pi\r', 'pi_start 4294967295\r',
    'pi_start 1\r', 'uptime\r', 'p', 'i\r', 'help\r',
]
script = ''.join(f'{i*1000000} {frame.encode().hex()}\n' for i,frame in enumerate(frames))
run = subprocess.run([sys.argv[1]], input=script, text=True, capture_output=True, check=True)
output = run.stdout
assert output.count('error: usage led_period <period_ms>') == 6, output
assert 'error: period_ms must be greater than 0' in output
assert 'led: blink, period 4294967295 ms' in output
assert 'led: off, period 4294967295 ms' in output
assert output.count('error: command longer than 63 characters or 4 arguments') == 2
assert 'pi: 4.000000000 in ' in output
assert 'pi: started, 4294967295 terms' in output
assert 'error: pi is busy' in output
assert 'pi: running' in output
assert output.count('uptime: ') == 2
assert 'help             list of commands' in output
print('PASS: numeric boundaries, errors without state changes, echo editing, partial frames, busy calculation')
