# Level selector for the standalone Windows build.
# Lists every level the engine can load, matching the web deploy's list, which is
# wider than the in-game menu: the faithful CMainMenu only routes to New Game and
# the eight VR levels, while --level reaches all of them.
Add-Type -AssemblyName System.Windows.Forms
Add-Type -AssemblyName System.Drawing
[System.Windows.Forms.Application]::EnableVisualStyles()

$root = Split-Path -Parent $MyInvocation.MyCommand.Path
$exe  = Join-Path $root 'jnengine.exe'
if (-not (Test-Path $exe)) {
    [System.Windows.Forms.MessageBox]::Show(
        "jnengine.exe not found next to this launcher.`n`nKeep level-select.ps1 in the same folder as the game.",
        'JN Engine', 'OK', 'Error') | Out-Null
    exit 1
}

# id, label, group -- the 25 story levels are exactly the web deploy's list
$levels = @(
    @('level1','Level 1','Story'),      @('level1a','Level 1A','Story'),
    @('level1b','Level 1B','Story'),    @('level1c','Level 1C','Story'),
    @('level1d','Level 1D','Story'),    @('level1e','Level 1E','Story'),
    @('level1f','Level 1F','Story'),    @('level2','Level 2','Story'),
    @('level2a','Level 2A','Story'),    @('level2b','Level 2B','Story'),
    @('level3','Level 3','Story'),      @('level3a','Level 3A','Story'),
    @('level3c','Level 3C','Story'),    @('level3d','Level 3D','Story'),
    @('level4','Level 4','Story'),      @('level4a','Level 4A','Story'),
    @('level4b','Level 4B','Story'),    @('level4c','Level 4C','Story'),
    @('level4d','Level 4D','Story'),    @('level5','Level 5','Story'),
    @('level5a','Level 5A','Story'),    @('level5b','Level 5B','Story'),
    @('level6','Level 6','Story'),      @('level6a','Level 6A','Story'),
    @('level7','Level 7','Story'),
    @('vr01','VR 01','VR challenge'),   @('vr02','VR 02','VR challenge'),
    @('vr03','VR 03','VR challenge'),   @('vr04','VR 04','VR challenge'),
    @('vr05','VR 05','VR challenge'),   @('vr06','VR 06','VR challenge'),
    @('vr07','VR 07','VR challenge'),   @('vr08','VR 08','VR challenge')
)

$accent = [System.Drawing.Color]::FromArgb(88, 166, 255)
$muted  = [System.Drawing.Color]::FromArgb(125, 136, 150)

$f = New-Object System.Windows.Forms.Form
$f.Text = 'Jimmy Neutron - choose a level'
$f.Size = New-Object System.Drawing.Size(440, 330)
$f.StartPosition = 'CenterScreen'
$f.FormBorderStyle = 'FixedDialog'
$f.MaximizeBox = $false
$f.BackColor = [System.Drawing.Color]::FromArgb(13, 17, 23)
$f.ForeColor = [System.Drawing.Color]::FromArgb(230, 237, 243)
$f.Font = New-Object System.Drawing.Font('Segoe UI', 9.5)

function Add-Label($text, $x, $y, $w, $colour) {
    $l = New-Object System.Windows.Forms.Label
    $l.Text = $text
    $l.Location = New-Object System.Drawing.Point($x, $y)
    $l.Size = New-Object System.Drawing.Size($w, 20)
    $l.ForeColor = $colour
    $f.Controls.Add($l)
    return $l
}

Add-Label 'Jimmy Neutron: Boy Genius' 20 16 390 $accent | Out-Null
Add-Label 'Pick a level and press Play. Quitting the game returns you here.' 20 38 390 $muted | Out-Null

Add-Label 'Level' 20 72 60 $muted | Out-Null
$cb = New-Object System.Windows.Forms.ComboBox
$cb.Location = New-Object System.Drawing.Point(20, 94)
$cb.Size = New-Object System.Drawing.Size(385, 26)
$cb.DropDownStyle = 'DropDownList'
$cb.BackColor = [System.Drawing.Color]::FromArgb(21, 27, 34)
$cb.ForeColor = $f.ForeColor
foreach ($l in $levels) { [void]$cb.Items.Add(('{0}   -   {1}' -f $l[1], $l[2])) }
$cb.SelectedIndex = 7      # Level 2, the web deploy's default
$f.Controls.Add($cb)

$noDmg = New-Object System.Windows.Forms.CheckBox
$noDmg.Text = 'No damage - enemies and hazards cannot kill you'
$noDmg.Location = New-Object System.Drawing.Point(20, 132)
$noDmg.Size = New-Object System.Drawing.Size(390, 22)
$noDmg.ForeColor = $f.ForeColor
$noDmg.Checked = $true
$f.Controls.Add($noDmg)

$menuBox = New-Object System.Windows.Forms.CheckBox
$menuBox.Text = 'Open the in-game main menu instead (New Game + VR)'
$menuBox.Location = New-Object System.Drawing.Point(20, 156)
$menuBox.Size = New-Object System.Drawing.Size(390, 22)
$menuBox.ForeColor = $muted
$f.Controls.Add($menuBox)

$status = Add-Label '' 20 240 390 $muted

$play = New-Object System.Windows.Forms.Button
$play.Text = 'Play'
$play.Location = New-Object System.Drawing.Point(20, 192)
$play.Size = New-Object System.Drawing.Size(120, 36)
$play.FlatStyle = 'Flat'
$play.BackColor = $accent
$play.ForeColor = [System.Drawing.Color]::FromArgb(6, 19, 31)
$play.Font = New-Object System.Drawing.Font('Segoe UI', 10, [System.Drawing.FontStyle]::Bold)
$f.Controls.Add($play)

$quit = New-Object System.Windows.Forms.Button
$quit.Text = 'Quit'
$quit.Location = New-Object System.Drawing.Point(152, 192)
$quit.Size = New-Object System.Drawing.Size(90, 36)
$quit.FlatStyle = 'Flat'
$quit.BackColor = [System.Drawing.Color]::FromArgb(21, 27, 34)
$quit.ForeColor = $f.ForeColor
$f.Controls.Add($quit)

$play.Add_Click({
    $sel = $levels[$cb.SelectedIndex]
    $argv = @()
    if ($menuBox.Checked) { $argv += '--menu' } else { $argv += @('--level', $sel[0]) }
    if ($noDmg.Checked)   { $argv += '--nodamage' }
    $status.Text = 'launching ' + ($argv -join ' ') + ' ...'
    $f.Refresh()
    try {
        $proc = Start-Process -FilePath $exe -ArgumentList $argv -WorkingDirectory $root -PassThru
        $f.WindowState = 'Minimized'
        $proc.WaitForExit()
        $f.WindowState = 'Normal'
        $f.Activate()
        $status.Text = 'exited (code ' + $proc.ExitCode + ')'
    } catch {
        $status.Text = 'failed: ' + $_.Exception.Message
    }
})
$quit.Add_Click({ $f.Close() })
$f.AcceptButton = $play

[void]$f.ShowDialog()
