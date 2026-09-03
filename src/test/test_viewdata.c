#include <check.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <gtk/gtk.h>

#include <wisp/utils/errors.h>

/* UI function stubs for testing viewdata helper logic */
nserror nsgtk_warning(const char *warning, const char *detail)
{
    (void)warning;
    (void)detail;
    return NSERROR_OK;
}

void nsgtk_about_dialog_init(GtkWindow *parent)
{
    (void)parent;
}

nserror nsgtk_builder_new_from_resname(const char *resname, GtkBuilder **builder_out)
{
    (void)resname;
    (void)builder_out;
    return NSERROR_OK;
}

void nsgtk_widget_modify_font(GtkWidget *widget, PangoFontDescription *font_desc)
{
    (void)widget;
    (void)font_desc;
}

GtkWidget *nsgtk_dialog_get_content_area(GtkDialog *dialog)
{
    (void)dialog;
    return NULL;
}

#include "gtk/viewdata.c"

START_TEST(test_xdg_get_exec_cmd_valid)
{
    char tmpdir[] = "/tmp/wisp_test_xdg_XXXXXX";
    ck_assert_ptr_ne(mkdtemp(tmpdir), NULL);

    char desktop_path[2048];
    snprintf(desktop_path, sizeof(desktop_path), "%s/applications", tmpdir);
    mkdir(desktop_path, 0755);

    snprintf(desktop_path, sizeof(desktop_path), "%s/applications/gedit.desktop", tmpdir);

    FILE *f = fopen(desktop_path, "w");
    ck_assert_ptr_ne(f, NULL);
    fputs("# Comment line\n", f);
    fputs("[Desktop Entry]\n", f);
    fputs("Type=Application\n", f);
    fputs("Name=Text Editor\n", f);
    fputs("Exec=gedit %U\n", f);
    fclose(f);

    char *exec_cmd = xdg_get_exec_cmd(tmpdir, "gedit.desktop");
    ck_assert_ptr_ne(exec_cmd, NULL);
    ck_assert_str_eq(exec_cmd, "gedit %U");
    free(exec_cmd);

    unlink(desktop_path);
    snprintf(desktop_path, sizeof(desktop_path), "%s/applications", tmpdir);
    rmdir(desktop_path);
    rmdir(tmpdir);
}
END_TEST

START_TEST(test_xdg_get_exec_cmd_spaces_around_equals)
{
    char tmpdir[] = "/tmp/wisp_test_xdg_XXXXXX";
    ck_assert_ptr_ne(mkdtemp(tmpdir), NULL);

    char desktop_path[2048];
    snprintf(desktop_path, sizeof(desktop_path), "%s/applications", tmpdir);
    mkdir(desktop_path, 0755);

    snprintf(desktop_path, sizeof(desktop_path), "%s/applications/editor.desktop", tmpdir);

    FILE *f = fopen(desktop_path, "w");
    ck_assert_ptr_ne(f, NULL);
    fputs("  [Desktop Entry]  \n", f);
    fputs("  Exec   =   /usr/bin/editor --open %f  \n", f);
    fclose(f);

    char *exec_cmd = xdg_get_exec_cmd(tmpdir, "editor.desktop");
    ck_assert_ptr_ne(exec_cmd, NULL);
    ck_assert_str_eq(exec_cmd, "/usr/bin/editor --open %f");
    free(exec_cmd);

    unlink(desktop_path);
    snprintf(desktop_path, sizeof(desktop_path), "%s/applications", tmpdir);
    rmdir(desktop_path);
    rmdir(tmpdir);
}
END_TEST

START_TEST(test_xdg_get_exec_cmd_group_selection)
{
    char tmpdir[] = "/tmp/wisp_test_xdg_XXXXXX";
    ck_assert_ptr_ne(mkdtemp(tmpdir), NULL);

    char desktop_path[2048];
    snprintf(desktop_path, sizeof(desktop_path), "%s/applications", tmpdir);
    mkdir(desktop_path, 0755);

    snprintf(desktop_path, sizeof(desktop_path), "%s/applications/multi_group.desktop", tmpdir);

    FILE *f = fopen(desktop_path, "w");
    ck_assert_ptr_ne(f, NULL);
    fputs("[Desktop Action NewWindow]\n", f);
    fputs("Exec=gedit --new-window\n", f);
    fputs("\n", f);
    fputs("[Desktop Entry]\n", f);
    fputs("Exec=gedit %U\n", f);
    fputs("\n", f);
    fputs("[Desktop Action Settings]\n", f);
    fputs("Exec=gedit --settings\n", f);
    fclose(f);

    char *exec_cmd = xdg_get_exec_cmd(tmpdir, "multi_group.desktop");
    ck_assert_ptr_ne(exec_cmd, NULL);
    ck_assert_str_eq(exec_cmd, "gedit %U");
    free(exec_cmd);

    unlink(desktop_path);
    snprintf(desktop_path, sizeof(desktop_path), "%s/applications", tmpdir);
    rmdir(desktop_path);
    rmdir(tmpdir);
}
END_TEST

START_TEST(test_xdg_get_exec_cmd_missing_desktop_entry_group)
{
    char tmpdir[] = "/tmp/wisp_test_xdg_XXXXXX";
    ck_assert_ptr_ne(mkdtemp(tmpdir), NULL);

    char desktop_path[2048];
    snprintf(desktop_path, sizeof(desktop_path), "%s/applications", tmpdir);
    mkdir(desktop_path, 0755);

    snprintf(desktop_path, sizeof(desktop_path), "%s/applications/no_entry.desktop", tmpdir);

    FILE *f = fopen(desktop_path, "w");
    ck_assert_ptr_ne(f, NULL);
    fputs("[Desktop Action NewWindow]\n", f);
    fputs("Exec=gedit --new-window\n", f);
    fclose(f);

    char *exec_cmd = xdg_get_exec_cmd(tmpdir, "no_entry.desktop");
    ck_assert_ptr_null(exec_cmd);

    unlink(desktop_path);
    snprintf(desktop_path, sizeof(desktop_path), "%s/applications", tmpdir);
    rmdir(desktop_path);
    rmdir(tmpdir);
}
END_TEST

START_TEST(test_xdg_get_default_app_valid)
{
    char tmpdir[] = "/tmp/wisp_test_xdg_XXXXXX";
    ck_assert_ptr_ne(mkdtemp(tmpdir), NULL);

    char defaults_path[2048];
    snprintf(defaults_path, sizeof(defaults_path), "%s/applications", tmpdir);
    mkdir(defaults_path, 0755);

    snprintf(defaults_path, sizeof(defaults_path), "%s/applications/defaults.list", tmpdir);

    FILE *f = fopen(defaults_path, "w");
    ck_assert_ptr_ne(f, NULL);
    fputs("[Default Applications]\n", f);
    fputs("text/plain = gedit.desktop;org.gnome.TextEditor.desktop;\n", f);
    fputs("image/png=eog.desktop;\n", f);
    fclose(f);

    char *app = xdg_get_default_app(tmpdir, "text/plain");
    ck_assert_ptr_ne(app, NULL);
    ck_assert_str_eq(app, "gedit.desktop");
    free(app);

    char *app_png = xdg_get_default_app(tmpdir, "image/png");
    ck_assert_ptr_ne(app_png, NULL);
    ck_assert_str_eq(app_png, "eog.desktop");
    free(app_png);

    unlink(defaults_path);
    snprintf(defaults_path, sizeof(defaults_path), "%s/applications", tmpdir);
    rmdir(defaults_path);
    rmdir(tmpdir);
}
END_TEST

START_TEST(test_build_exec_argv_parsing)
{
    char **argv = build_exec_argv("/tmp/file.txt", "gedit --wait %f");
    ck_assert_ptr_ne(argv, NULL);
    ck_assert_str_eq(argv[0], "gedit");
    ck_assert_str_eq(argv[1], "--wait");
    ck_assert_str_eq(argv[2], "/tmp/file.txt");
    ck_assert_ptr_null(argv[3]);

    for (int i = 0; argv[i] != NULL; i++) {
        free(argv[i]);
    }
    free(argv);
}
END_TEST

static Suite *viewdata_suite_create(void)
{
    Suite *s = suite_create("Viewdata XDG Parsing");
    TCase *tc = tcase_create("XDG Desktop Entry");

    tcase_add_test(tc, test_xdg_get_exec_cmd_valid);
    tcase_add_test(tc, test_xdg_get_exec_cmd_spaces_around_equals);
    tcase_add_test(tc, test_xdg_get_exec_cmd_group_selection);
    tcase_add_test(tc, test_xdg_get_exec_cmd_missing_desktop_entry_group);
    tcase_add_test(tc, test_xdg_get_default_app_valid);
    tcase_add_test(tc, test_build_exec_argv_parsing);

    suite_add_tcase(s, tc);
    return s;
}

int main(void)
{
    int number_failed;
    SRunner *sr = srunner_create(viewdata_suite_create());

    srunner_run_all(sr, CK_ENV);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
