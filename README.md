# 📂 Repo de Electrónica Digital 3

### (no pases el .zip por WhatsApp)

Este repo existe por una razón simple: dejar de mandarnos archivos por todos lados y tener el código en un solo lugar. Entonces podemos:

- Compartir código en C sin vueltas
- Tener siempre la última versión a mano
- Evitar el clásico `final_final_v3_posta_estavez.c`

---

## REGLA DE ORO

**No suban archivos de configuracion, solo los .c**

vayan a ver [como se suben los archivos](#subir-cambios)

---

## 🚀 Cómo usar

### Clonar (la primera vez)

```bash
git clone <URL_DEL_REPO>
cd <NOMBRE_DEL_REPO>
```

---

### Antes de ponerte a laburar

```bash
git pull origin main
```

Traer la ultima version subida a github, asi no nos pisamos el codigo entre nosotros.

Si vamos a laburar muchos a la vez capaz conviene hacer ramas pero eso vamos viendo.

---

### Subir cambios

⚠️ **No uses `git add .`** — eso agrega todo, incluyendo la mugre que queremos ignorar.
Solo agregá el `.c` que modificaste:

> **Recomendacion**, no usen la terminal sino una interfaz grafica de git que les deje agregar archivos con un click, si quieren usar terminal ahi les va:

```bash
git add ruta/del/archivo.c
```

💡 Tip: podés escribir parte del nombre y apretar **TAB** para autocompletar el path.

Después:

```bash
git commit -m "Descripción breve de qué hiciste"
git push origin main
```

Para el mensaje del commit, algo útil como `"Agrego función de delay"` o `"Corrijo inicialización del timer"`. No tiene que ser una novela, pero tampoco `"cosas"` o `"asd"`.

---

### La cague e hice `git add .`

Hace `git reset` si no comiteaste nada, si hiciste commit tenes que hacer `git reset HEAD~1`

---

### Esto es para que nos organicemos mejor, no es la idea que reniegen con el repo, cualquier duda mandenme

_Con amor: Tino_

---
