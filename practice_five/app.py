from flask import Flask, render_template, request
import subprocess

app = Flask(__name__, template_folder=".")

@app.route('/')
def home():
    return render_template('index.html')

@app.route('/submit', methods=['POST'])
def submit():
    code = request.form['code']
    inp = request.form['input']
    check = request.form.get('check')

    cmd = ["docker", "run", "gcc:latest"]
    if check == '1':
        cmd.extend(["sh", "-c", f"echo '{code}' >> program.c | gcc program.c && ./a.out '{inp}'"])
    else:
        cmd.extend(["sh", "-c", f"echo '{code}' >> program.c | gcc program.c && ./a.out"])
    
    res = subprocess.run(cmd, capture_output=True)
    out = (res.stderr if res.stderr else res.stdout).decode("utf-8")
    return render_template('index.html', code=code, input=inp, out=out, check=check)

if __name__ == '__main__':
    app.run(debug=True)