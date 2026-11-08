# Génère les fichiers SVG du logo Eolia (symbole, logo horizontal, favicon).
# Usage : python3 generer_logo.py <dossier_de_sortie>
import math, sys
NUIT="#0F2E3D"; ALIZE="#1C8C86"; COURANT="#F2A93B"
def blade(c):
    # pale profilée pointant vers le haut, bord d'attaque bombé
    return f'<path d="M57 50 C49 38 47 18 58 4 C60 2 63 3 63 6 C66 22 67 38 64 50 Z" fill="{c}"/>'
def rotor(cb, chub, cled, arc=True, carc=None):
    s='<g>'
    for a in (0,120,240):
        s+=f'<g transform="rotate({a} 60 60)">{blade(cb)}</g>'
    s+=f'<circle cx="60" cy="60" r="10" fill="{chub}"/><circle cx="60" cy="60" r="4" fill="{cled}"/>'
    if arc:
        carc=carc or cb
        # arc de supervision : trajectoire des pales, terminé par un noeud
        r=53; a0=math.radians(292); a1=math.radians(18)
        x0,y0=60+r*math.cos(a0),60+r*math.sin(a0); x1,y1=60+r*math.cos(a1),60+r*math.sin(a1)
        s+=f'<path d="M{x0:.2f} {y0:.2f} A{r} {r} 0 0 1 {x1:.2f} {y1:.2f}" fill="none" stroke="{carc}" stroke-width="3.5" stroke-linecap="round"/>'
        s+=f'<circle cx="{x1:.2f}" cy="{y1:.2f}" r="5.5" fill="{carc}"/>'
    return s+'</g>'
def word(c, cdot):
    sw=10; r=20; y=75
    e=25; o=87; l=129; i=151; a=193
    p=[]
    ex,ey=e+r*math.cos(math.radians(45)), y+r*math.sin(math.radians(45))
    p.append(f'<path d="M{e-r} {y} H{e+r} A{r} {r} 0 1 0 {ex:.2f} {ey:.2f}"/>')
    p.append(f'<circle cx="{o}" cy="{y}" r="{r}"/>')
    p.append(f'<path d="M{l} 28 V100"/>')
    p.append(f'<path d="M{i} 57 V100"/>')
    p.append(f'<circle cx="{a}" cy="{y}" r="{r}"/><path d="M{a+r} 57 V100"/>')
    g=f'<g fill="none" stroke="{c}" stroke-width="{sw}" stroke-linecap="round" stroke-linejoin="round">'+''.join(p)+'</g>'
    g+=f'<circle cx="{i}" cy="36" r="6.5" fill="{cdot}"/>'
    return g
def svg(w,h,body,title):
    return f'<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 {w} {h}" width="{w*4}" height="{h*4}" role="img" aria-label="{title}"><title>{title}</title>{body}</svg>\n'
variants={
 'couleur':(ALIZE,NUIT,COURANT,NUIT,COURANT,NUIT),
 'blanc':("#FFFFFF","#FFFFFF",COURANT,"#FFFFFF",COURANT,"#FFFFFF"),
 'noir':("#000000","#000000","#FFFFFF","#000000","#000000","#000000"),
}
out=sys.argv[1]
for k,(cb,chub,cled,cw,cdot,carc) in variants.items():
    sym=rotor(cb,chub,cled,True,carc)
    open(f'{out}/eolia_symbole_{k}.svg','w').write(svg(120,120,sym,'Eolia'))
    # logo horizontal : symbole 120 + texte décalé
    body=f'{sym}<g transform="translate(142 -2) scale(0.92)">{word(cw,cdot)}</g>'
    open(f'{out}/eolia_logo_{k}.svg','w').write(svg(350,120,body,'Eolia'))
# favicon : sans arc
open(f'{out}/eolia_favicon.svg','w').write(svg(120,120,rotor(ALIZE,NUIT,COURANT,False),'Eolia'))
