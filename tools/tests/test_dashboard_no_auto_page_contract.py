from contract_helpers import check, read_repo_text


DASHBOARD_VIEW = "src/firmware/bikemb/src/app/dashboard_view.cpp"
DASHBOARD_CORE_HEADER = "src/firmware/bikemb/src/app/dashboard_view_core.h"
DEMO_METRICS_HEADER = "src/firmware/bikemb/src/app/demo_metrics.h"
DEMO_METRICS_SOURCE = "src/firmware/bikemb/src/app/demo_metrics.cpp"


def test_dashboard_pages_do_not_auto_advance_from_demo_metrics() -> None:
    view = read_repo_text(DASHBOARD_VIEW)
    core_header = read_repo_text(DASHBOARD_CORE_HEADER)
    metrics_header = read_repo_text(DEMO_METRICS_HEADER)
    metrics_source = read_repo_text(DEMO_METRICS_SOURCE)

    combined = "\n".join([view, core_header, metrics_header, metrics_source])
    check("activePage" not in combined, "Demo metrics must not expose or update an automatic page field.")
    check("(now / 5000) % 3" not in metrics_source, "Dashboard demo data must not auto-cycle pages by time.")


if __name__ == "__main__":
    test_dashboard_pages_do_not_auto_advance_from_demo_metrics()
    print("PASS test_dashboard_no_auto_page_contract")
