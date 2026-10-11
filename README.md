# Shlose – De-Esser (VST3)

## בניית הפלאגין בענן (בלי להתקין כלום)
1. פתח חשבון חינמי ב-github.com ולחץ New repository (למשל בשם Shlose).
2. העלה את כל תוכן התיקייה הזו (כולל התיקייה הנסתרת `.github`). דרך אתר GitHub: Add file ← Upload files וגרור הכל.
3. עבור ללשונית **Actions**. הבנייה מתחילה לבד (כ-5–10 דקות).
4. כשהיא מסתיימת (וי ירוק), היכנס אליה, ובתחתית העמוד תחת **Artifacts** הורד את `Shlose-VST3-Windows` או `Shlose-VST3-macOS`.
5. פתח את ה-zip והעתק את התיקייה `Shlose.vst3` ל:
   - Windows: `C:\Program Files\Common Files\VST3`
   - Mac: `/Library/Audio/Plug-Ins/VST3`
6. באברטון: Preferences ← Plug-Ins ← Rescan. הפלאגין יופיע תחת VST3.

ב-Mac, אם המערכת חוסמת את הקובץ, הרץ בטרמינל: `xattr -cr /Library/Audio/Plug-Ins/VST3/Shlose.vst3`

## הפקדים
- **Frequency / Q** – מרכז ורוחב אזור ה-S (בדרך כלל 5–9 kHz)
- **Threshold** – מעל איזו רמה מתחיל הטיפול
- **Ratio / Range** – עוצמת הדיכוי והמקסימום שמותר לו להוריד
- **Attack / Release** – מהירות התגובה
- **Mode** – Split-band מדכא רק את אזור התדרים; Wideband מוריד את כל הסיגנל
- **Listen** – שומעים רק את אזור הזיהוי, כדי לכוון Frequency בקלות
- **Mix / Output** – ערבוב וגיין יציאה
