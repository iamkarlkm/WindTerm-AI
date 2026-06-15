// Prevents additional console window on Windows in release, DO NOT REMOVE!!
#![cfg_attr(not(debug_assertions), windows_subsystem = "windows")]

use tauri::Manager;

#[tauri::command]
fn get_app_version() -> String {
    "1.0.0".to_string()
}

#[tauri::command]
fn get_snippet_content(path: String) -> Result<String, String> {
    std::fs::read_to_string(&path).map_err(|e| e.to_string())
}

#[tauri::command]
fn save_snippet_content(path: String, content: String) -> Result<(), String> {
    std::fs::write(&path, content).map_err(|e| e.to_string())
}

fn main() {
    tauri::Builder::default()
        .plugin(tauri_plugin_shell::init())
        .plugin(tauri_plugin_dialog::init())
        .plugin(tauri_plugin_clipboard_manager::init())
        .plugin(tauri_plugin_global_shortcut::Builder::new().build())
        .plugin(tauri_plugin_fs::init())
        .invoke_handler(tauri::generate_handler![
            get_app_version,
            get_snippet_content,
            save_snippet_content
        ])
        .setup(|app| {
            // 设置窗口标题
            let window = app.get_webview_window("main").unwrap();
            window.set_title("WindTerm AI - Web Terminal Client").unwrap();

            // 系统托盘
            use tauri::tray::{TrayIconBuilder, MenuEvent};
            let _tray = TrayIconBuilder::new()
                .tooltip("WindTerm AI")
                .on_menu_event(|app, event| match event.id.as_ref() {
                    "show" => {
                        if let Some(window) = app.get_webview_window("main") {
                            let _ = window.show();
                            let _ = window.set_focus();
                        }
                    }
                    "quit" => {
                        app.exit(0);
                    }
                    _ => {}
                })
                .build(app)?;

            Ok(())
        })
        .run(tauri::generate_context!())
        .expect("启动 WindTerm AI 客户端时出错");
}
