import matplotlib as mp
import matplotlib.pyplot as plt

from .formatting import set_size


def front_plot(Xs, Ys, title: str, current_uuid: str, save_path="", maxX=True, maxY=True):
    plt.style.use("tableau-colorblind10")
    mp.use("pgf")
    mp.rcParams.update({
        "pgf.texsystem": "pdflatex",
        'font.family': 'serif',
        'text.usetex': True,
        'pgf.rcfonts': False,
    })
    fig, ax = plt.subplots(1, 1, figsize=set_size(506/2))
    """Pareto frontier selection process"""
    sorted_list = sorted([[Xs[i], Ys[i]] for i in range(len(Xs))], reverse=maxY)
    pareto_front = [sorted_list[0]]
    for pair in sorted_list[1:]:
        if maxY:
            if pair[1] >= pareto_front[-1][1]:
                pareto_front.append(pair)
        else:
            if pair[1] <= pareto_front[-1][1]:
                pareto_front.append(pair)

    '''Plotting process'''
    ax.scatter(Xs, Ys)
    pf_X = [pair[0] for pair in pareto_front]
    pf_Y = [pair[1] for pair in pareto_front]
    ax.plot(pf_X, pf_Y)
    ax.set_xlabel("Frequency (msg/s)")
    ax.set_ylabel("DTW Distance")
    ax.set_title(label=title)
    fig.tight_layout()
    # plt.show()
    plt.savefig(f"{save_path}-{title}-{current_uuid}.pgf", format='pgf')
