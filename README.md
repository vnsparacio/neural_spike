neural_spike
============

C-code to interface with a raspberry pi and front end analog-to-digital converter. Correctly takes in inputs from neurons, analyzes electrical spikes, and induces positive feedback if inputs match a constant pattern. Also breaks analysis and stimulation into specifically timed chunks for consistency, e.g. 500ms blocks to record and stimulate.
