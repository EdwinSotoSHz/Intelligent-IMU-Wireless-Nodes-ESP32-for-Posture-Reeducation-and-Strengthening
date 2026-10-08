// js/app.js
document.addEventListener("DOMContentLoaded", function () {
    var navbar = document.getElementById('mainNavbar');

    // 1. Glassmorphism scroll effect
    function onScroll() {
        if (window.scrollY > 60) {
            navbar.classList.add('navbar-scrolled');
        } else {
            navbar.classList.remove('navbar-scrolled');
        }
    }

    window.addEventListener('scroll', onScroll, { passive: true });
    onScroll();

    // 2. Active nav-link highlighter (IntersectionObserver)
    var navLinks = document.querySelectorAll('.navbar-nav .nav-link[href^="#"]');
    var sections = [];

    navLinks.forEach(function (link) {
        var target = document.querySelector(link.getAttribute('href'));
        if (target) sections.push(target);
    });

    if (sections.length > 0 && 'IntersectionObserver' in window) {
        var observer = new IntersectionObserver(function (entries) {
            entries.forEach(function (entry) {
                if (entry.isIntersecting) {
                    navLinks.forEach(function (l) { l.classList.remove('active'); });
                    var activeLink = document.querySelector(
                        '.navbar-nav .nav-link[href="#' + entry.target.id + '"]'
                    );
                    if (activeLink) activeLink.classList.add('active');
                }
            });
        }, {
            rootMargin: '-20% 0px -60% 0px',
            threshold: 0
        });

        sections.forEach(function (section) { observer.observe(section); });
    }

    // 3. Animaciones de entrada al hacer scroll ([data-reveal])
    var reduceMotion = window.matchMedia('(prefers-reduced-motion: reduce)').matches;
    var revealEls = document.querySelectorAll('[data-reveal]');

    if (!reduceMotion && 'IntersectionObserver' in window) {
        var revealObserver = new IntersectionObserver(function (entries, obs) {
            entries.forEach(function (entry) {
                if (entry.isIntersecting) {
                    entry.target.classList.add('revealed');
                    obs.unobserve(entry.target);
                }
            });
        }, { rootMargin: '0px 0px -8% 0px', threshold: 0.08 });

        revealEls.forEach(function (el) {
            el.classList.add('reveal-init');
            revealObserver.observe(el);
        });
    }

    // 4. Carrusel: sin auto-avance si el usuario prefiere menos movimiento
    var carouselEl = document.getElementById('dashboardCarousel');
    if (reduceMotion && carouselEl && window.bootstrap) {
        bootstrap.Carousel.getOrCreateInstance(carouselEl).pause();
    }
});